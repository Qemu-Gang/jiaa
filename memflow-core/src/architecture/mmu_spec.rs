#[macro_use]
pub mod masks;
use masks::*;

use super::translate_data::TranslateData;
use crate::iter::SplitAtIndex;
use crate::types::{Address, PageType, PhysicalAddress};
use crate::vtop_trace;
use crate::Error;

/// The `ArchMMUSpec` structure defines how a real memory management unit should behave when
/// translating virtual memory addresses to physical ones.
///
/// The core logic of virtual to physical memory translation is practically the same, but different
/// MMUs may have different address space sizes, and thus split the addresses in different ways.
///
/// For instance, most x86_64 architectures have 4 levels of page mapping, providing 52-bit address
/// space. Virtual address gets split into 4 9-bit regions, and a 12-bit one, the first 4 are used
/// to index the page tables, and the last, 12-bit split is used as an offset to get the final
/// memory address. Meanwhile, x86 with PAE has 3 levels of page mapping, providing 36-bit address
/// space. Virtual address gets split into a 2-bit, 2 9-bit and a 12-bit regions - the last one is
/// also used as an offset from the physical frame. The difference is of level count, and virtual
/// address splits, but the core page table walk stays the same.
///
/// Our virtual to physical memory ranslation code is the same for both architectures, in fact, it
/// is also the same for the x86 (non-PAE) architecture that has different PTE and pointer sizes.
/// All that differentiates the translation process is the data inside this structure.
pub struct ArchMMUSpec {
    /// defines the way virtual addresses gets split (the last element
    /// being the final physical page offset, and thus treated a bit differently)
    pub virtual_address_splits: &'static [u8],
    /// defines at which page mapping steps we can return a large page.
    /// Steps are indexed from 0, and the list has to be sorted, otherwise the code may fail.
    pub valid_final_page_steps: &'static [usize],
    /// define the address space upper bound (32 for x86, 52 for x86_64)
    pub address_space_bits: u8,
    /// native pointer size in bytes for the architecture.
    pub addr_size: u8,
    /// size of an individual page table entry in bytes.
    pub pte_size: usize,
    /// index of a bit in PTE defining whether the page is present or not.
    pub present_bit: u8,
    /// index of a bit in PTE defining if the page is writeable.
    pub writeable_bit: u8,
    /// index of a bit in PTE defining if the page is non-executable.
    pub nx_bit: u8,
    /// index of a bit in PTE defining if the PTE points to a large page.
    pub large_page_bit: u8,
}

impl ArchMMUSpec {
    /// Mask a page table entry address to retrieve the next page table entry
    ///
    /// This function uses virtual_address_splits to mask the first bits out in `pte_addr`, but
    /// keep everything else until the `address_space_bits` upper bound.
    ///
    /// # Arguments
    ///
    /// * `pte_addr` - page table entry address to mask
    /// * `step` - the current step in the page walk
    ///
    /// # Remarks
    ///
    /// The final step is handled differently, because the final split provides a byte offset to
    /// the page, instead of an offset that has to be multiplied by `pte_size`. We do that by
    /// subtracting `pte_size` logarithm from the split size.
    pub fn pte_addr_mask(&self, pte_addr: Address, step: usize) -> u64 {
        let max = self.address_space_bits - 1;
        let min = self.virtual_address_splits[step]
            + if step == self.virtual_address_splits.len() - 1 {
                0
            } else {
                self.pte_size.to_le().trailing_zeros() as u8
            };
        let mask = make_bit_mask(min, max);
        vtop_trace!("pte_addr_mask={:b}", mask);
        pte_addr.as_u64() & mask
    }

    /// Filter out the input virtual address range to be in bounds
    ///
    ///
    /// # Arguments
    ///
    /// * `(addr, buf)` - an address and buffer pair that gets split and filtered
    /// * `valid_out` - output collection that contains valid splits
    /// * `fail_out` - the final collection where the function will push rejected ranges to
    ///
    /// # Remarks
    ///
    /// This function cuts the input virtual address to be inside range `(-2^address_space_bits;
    /// +2^address_space_bits)`. It may result in 2 ranges, and it may have up to 2 failed ranges
    pub fn virt_addr_filter<B, VO, FO>(
        &self,
        (addr, buf): (Address, B),
        valid_out: &mut VO,
        fail_out: &mut FO,
    ) where
        B: SplitAtIndex,
        VO: Extend<TranslateData<B>>,
        FO: Extend<(Error, Address, B)>,
    {
        let mut tr_data = TranslateData { addr, buf };

        let (mut left, reject) =
            tr_data.split_inclusive_at(make_bit_mask(0, self.addr_size * 8 - 1) as usize);

        if let Some(data) = reject {
            fail_out.extend(Some((Error::VirtualTranslate, data.addr, data.buf)));
        }

        let virt_range = 1usize << (self.virt_addr_bit_range(0).1 - 1);

        let (lower, higher) = left.split_at(virt_range);

        if lower.length() > 0 {
            valid_out.extend(Some(lower).into_iter());
        }

        if let Some(mut data) = higher {
            let (reject, higher) = data.split_at_rev(virt_range);

            if higher.length() > 0 {
                valid_out.extend(Some(higher).into_iter());
            }

            if let Some(data) = reject {
                fail_out.extend(Some((Error::VirtualTranslate, data.addr, data.buf)));
            }
        }
    }

    fn virt_addr_bit_range(&self, step: usize) -> (u8, u8) {
        let max_index_bits = self.virtual_address_splits[step..].iter().sum::<u8>();
        let min_index_bits = max_index_bits - self.virtual_address_splits[step];
        (min_index_bits, max_index_bits)
    }

    fn virt_addr_to_pte_offset(&self, virt_addr: Address, step: usize) -> u64 {
        let (min, max) = self.virt_addr_bit_range(step);
        vtop_trace!("virt_addr_bit_range for step {} = ({}, {})", step, min, max);

        let shifted = virt_addr.as_u64() >> min;
        let mask = make_bit_mask(0, max - min - 1);

        (shifted & mask) * self.pte_size as u64
    }

    fn virt_addr_to_page_offset(&self, virt_addr: Address, step: usize) -> u64 {
        let max = self.virt_addr_bit_range(step).1;
        virt_addr.as_u64() & make_bit_mask(0, max - 1)
    }

    /// Return the number of splits of virtual addresses
    ///
    /// The returned value will be one more than the number of page table levels
    pub fn split_count(&self) -> usize {
        self.virtual_address_splits.len()
    }

    /// Calculate the size of the page table entry leaf in bytes
    ///
    /// This will return the number of page table entries at a specific step multiplied by the
    /// `pte_size`. Usually this will be an entire page, but in certain cases, like the highest
    /// mapping level of x86 with PAE, it will be less.
    ///
    /// # Arguments
    ///
    /// * `step` - the current step in the page walk
    pub fn pt_leaf_size(&self, step: usize) -> usize {
        let (min, max) = self.virt_addr_bit_range(step);
        (1 << (max - min)) * self.pte_size
    }

    /// Perform a virtual translation step, returning the next PTE address to read
    ///
    /// # Arguments
    ///
    /// * `pte_addr` - input PTE address that was read the last time (or DTB)
    /// * `virt_addr` - virtual address we are translating
    /// * `step` - the current step in the page walk
    pub fn vtop_step(&self, pte_addr: Address, virt_addr: Address, step: usize) -> Address {
        Address::from(
            self.pte_addr_mask(pte_addr, step) | self.virt_addr_to_pte_offset(virt_addr, step),
        )
    }

    /// Get the page size of a specific step without checking if such page could exist
    ///
    /// # Arguments
    ///
    /// * `step` - the current step in the page walk
    pub fn page_size_step_unchecked(&self, step: usize) -> usize {
        let max_index_bits = self.virtual_address_splits[step..].iter().sum::<u8>();
        (1u64 << max_index_bits) as usize
    }

    /// Get the page size of a specific page walk step
    ///
    /// This function is preferable to use externally, because in debug builds it will check if such
    /// page could exist, and if can not, it will panic
    ///
    /// # Arguments
    ///
    /// * `step` - the current step in the page walk
    pub fn page_size_step(&self, step: usize) -> usize {
        debug_assert!(self.valid_final_page_steps.binary_search(&step).is_ok());
        self.page_size_step_unchecked(step)
    }

    /// Get the page size of a specific mapping level
    ///
    /// This function is the same as `page_size_step`, but the `level` almost gets inverted. It
    /// goes in line with x86 page level naming. With 1 being the 4kb page, and higher meaning
    /// larger page.
    ///
    /// # Arguments
    ///
    /// * `level` - page mapping level to get the size of (1 meaning the smallest page)
    pub fn page_size_level(&self, level: usize) -> usize {
        self.page_size_step(self.virtual_address_splits.len() - level)
    }

    /// Get the final physical page
    ///
    /// This performs the final step of a successful translation - retrieve the final physical
    /// address. It does not perform any present checks, and assumes `pte_addr` points to a valid
    /// page.
    ///
    /// # Arguments
    ///
    /// * `pte_addr` - the address inside the previously read PTE
    /// * `virt_addr` - the virtual address we are currently translating
    /// * `step` - the current step in the page walk
    pub fn get_phys_page(
        &self,
        pte_addr: Address,
        virt_addr: Address,
        step: usize,
    ) -> PhysicalAddress {
        let phys_addr = Address::from(
            self.pte_addr_mask(pte_addr, step) | self.virt_addr_to_page_offset(virt_addr, step),
        );

        PhysicalAddress::with_page(
            phys_addr,
            PageType::from_writeable_bit(pte_addr.bit_at(self.writeable_bit)),
            self.page_size_step(step),
        )
    }

    /// Check if the current page table entry is valid
    ///
    /// # Arguments
    ///
    /// * `pte_addr` - current page table entry
    /// * `step` - the current step in the page walk
    pub fn check_entry(&self, pte_addr: Address, step: usize) -> bool {
        step == 0 || pte_addr.bit_at(self.present_bit)
    }

    /// Check if the current page table entry contains a physical page
    ///
    /// This will check `valid_final_page_steps` to determine if the PTE could have a large page,
    /// and then check the large page bit for confirmation. It will always return true on the final
    /// mapping regarding of the values in `valid_final_page_steps`. The `valid_final_page_steps`
    /// list has to be sorted for the function to work properly, because it uses binary search.
    ///
    /// # Arguments
    ///
    /// * `pte_addr` - current page table entry
    /// * `step` - the current step the page walk
    pub fn is_final_mapping(&self, pte_addr: Address, step: usize) -> bool {
        (step == self.virtual_address_splits.len() - 1)
            || (pte_addr.bit_at(self.large_page_bit)
                && self.valid_final_page_steps.binary_search(&step).is_ok())
    }
}
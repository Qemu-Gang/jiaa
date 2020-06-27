use crate::error::{Error, Result};
use crate::types::Address;

use std::default::Default;
use std::fmt;
use std::prelude::v1::*;

#[derive(Clone)]
pub struct MemoryMap {
    mappings: Vec<MemoryMapping>,
}

#[derive(Clone)]
struct MemoryMapping {
    base: Address,
    size: usize,
    real_base: Address,
}

impl Default for MemoryMap {
    fn default() -> Self {
        Self {
            mappings: Vec::new(),
        }
    }
}

impl MemoryMap {
    pub fn new() -> Self {
        MemoryMap::default()
    }

    /// Adds a new memory mapping to this memory map
    pub fn push(&mut self, base: Address, size: usize, real_base: Address) {
        // TODO: sort by base
        self.mappings.push(MemoryMapping {
            base,
            size,
            real_base,
        })
    }

    /// Maps a linear address to a hardware address.
    /// Returns an `Error::Bounds` error if the address does not lie within any memory region.
    pub fn map(&mut self, addr: Address) -> Result<Address> {
        let mapping = self
            .mappings
            .iter()
            .find(|m| m.base <= addr && addr < m.base + m.size)
            .ok_or_else(|| Error::Bounds)?;

        if mapping.base == mapping.real_base {
            Ok(addr)
        } else {
            Ok(mapping.real_base + (addr - mapping.base))
        }
    }
}

impl fmt::Debug for MemoryMap {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        for m in self.mappings.iter() {
            write!(f, "{:?}", m)?;
        }
        Ok(())
    }
}

impl fmt::Debug for MemoryMapping {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "MemoryMapping: base={:x} size={:x} real_base={:x}",
            self.base, self.size, self.real_base
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_mapping() {
        let mut map = MemoryMap::new();
        map.push(0x1000.into(), 0x1000, 0.into());
        map.push(0x3000.into(), 0x1000, 0x2000.into());

        assert_eq!(map.map(0x00ff.into()).is_err(), true);
        assert_eq!(map.map(0x10ff.into()), Ok(Address::from(0x00ff)));
        assert_eq!(map.map(0x20ff.into()).is_err(), true);
        assert_eq!(map.map(0x30ff.into()), Ok(Address::from(0x20ff)));
        assert_eq!(map.map(0x3000.into()), Ok(Address::from(0x2000)));
        assert_eq!(map.map(0x3fff.into()), Ok(Address::from(0x2fff)));
        assert_eq!(map.map(0x4000.into()).is_err(), true);
        assert_eq!(map.map(0x40ff.into()).is_err(), true);
    }
}
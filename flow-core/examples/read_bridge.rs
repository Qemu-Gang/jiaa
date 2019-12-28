use clap::{App, Arg};
use std::time::Instant;

use flow_core::address::{Address, Length};
use flow_core::*;

use flow_core::connector::bridge::client::BridgeClient;
use flow_core::mem::PhysicalRead;

fn main() {
    let argv = App::new("examples/bridge_read")
        .version("0.1")
        .arg(
            Arg::with_name("bridge-url")
                .short("url")
                .long("bridge-url")
                .value_name("URL")
                .help("bridge socket url")
                .takes_value(true),
        )
        .get_matches();

    let url = argv
        .value_of("bridge-url")
        .unwrap_or("unix:/tmp/qemu-connector-bridge");
    let mut bridge = match BridgeClient::connect(url) {
        Ok(s) => s,
        Err(e) => {
            println!("couldn't connect to bridge: {:?}", e);
            return;
        }
    };

    let mem = bridge.phys_read(Address::from(0x1000), len!(8)).unwrap();
    println!("Received memory: {:?}", mem);

    //bridge.read_registers().unwrap();

    let start = Instant::now();
    let mut counter = 0;
    loop {
        //let r = bridge.read_memory(0x1000, 0x1000).unwrap();
        //bridge.write_memory(0x1000, &r).unwrap();
        bridge
            .phys_read(Address::from(0x1000), len!(0x1000))
            .unwrap();

        counter += 1;
        if (counter % 10000) == 0 {
            let elapsed = start.elapsed().as_millis() as f64;
            if elapsed > 0.0 {
                println!("{} reads/sec", (f64::from(counter)) / elapsed * 1000.0);
                println!(
                    "{} reads/frame",
                    (f64::from(counter)) / elapsed * 1000.0 / 60.0
                );
                println!("{} ms/read", elapsed / (f64::from(counter)));
            }
        }
    }
}
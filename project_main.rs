use std::fs::{OpenOptions};
use std::io::{Read, Write};
use std::thread;
use std::time;

fn main()
{
    let mut led1;
    let mut led2;
    let mut led3;
    let mut clicks;

    let mut led1_status = String::new();
    let mut led2_status = String::new();
    let mut led3_status = String::new();
    let mut clicks_count = String::new();
    
    loop
    {
		led1 = OpenOptions::new().read(true).write(true).open("/sys/kernel/project/led1").expect("Failed to open file");
		led2 = OpenOptions::new().read(true).write(true).open("/sys/kernel/project/led2").expect("Failed to open file");
		led3 = OpenOptions::new().read(true).write(true).open("/sys/kernel/project/led3").expect("Failed to open file");
		clicks = OpenOptions::new().read(true).write(true).open("/sys/kernel/project/clicks").expect("Failed to open file");

		led1.read_to_string(&mut led1_status).expect("");
			led2.read_to_string(&mut led2_status).expect("");
		led3.read_to_string(&mut led3_status).expect("");
		clicks.read_to_string(&mut clicks_count).expect("");
		println!("{},{},{}, \n Clicks Count: {}", led1_status, led2_status, led3_status, clicks_count);

		match clicks_count.trim().parse().unwrap(){
			0 => {led1.write_all(b"0"); led2.write_all(b"0"); led3.write_all(b"0");},
			1..=5 => {led1.write_all(b"25"); led2.write_all(b"0"); led3.write_all(b"0");},
			6..=10 => {led1.write_all(b"50"); led2.write_all(b"0"); led3.write_all(b"0");},
			11..=15 => {led1.write_all(b"75"); led2.write_all(b"0"); led3.write_all(b"0");},
			16..=20 => {led1.write_all(b"100"); led2.write_all(b"0"); led3.write_all(b"0");},
			
			21..=25 => {led1.write_all(b"100"); led2.write_all(b"25"); led3.write_all(b"0");},
			26..=30 => {led1.write_all(b"100"); led2.write_all(b"50"); led3.write_all(b"0");},
			31..=35 => {led1.write_all(b"100"); led2.write_all(b"75"); led3.write_all(b"0");},
			36..=40 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"0");},
			
			41..=45 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"25");},
			46..=50 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"50");},
			51..=55=> {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"75");},
			56..=60 => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"100");},

			_ => {led1.write_all(b"100"); led2.write_all(b"100"); led3.write_all(b"100");}
		}
		// Resetting Count
		clicks.write_all(b"0");
		
		led1_status.clear();
		led2_status.clear();
		led3_status.clear();
		clicks_count.clear();

		// Closing files
		drop(led1);drop(led2);drop(led3);drop(clicks);

		thread::sleep(time::Duration::from_millis(10000));
    }
}
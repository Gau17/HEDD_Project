use std::fs::{OpenOptions};
use std::io::{Read, Write};
use std::thread;
use std::time;

fn main()
{
    let mut file_status = String::new();
    
    loop
    {
		let mut file = OpenOptions::new().read(true).write(true).open("/dev/project").expect("Failed to open file");

		file.read_to_string(&mut file_status).expect("");
		println!("{}", file_status);

		match clicks_count.trim().parse().unwrap(){
			0 => {file.write_all(b"Led1_intensity=0"); file.write_all(b"Led2_intensity=0"); file.write_all(b"Led3_intensity=0");},
			1..=5 => {file.write_all(b"Led1_intensity=25"); file.write_all(b"Led2_intensity=0"); file.write_all(b"Led3_intensity=0");},
			6..=10 => {file.write_all(b"Led1_intensity=50"); file.write_all(b"Led2_intensity=0"); file.write_all(b"Led3_intensity=0");},
			11..=15 => {file.write_all(b"Led1_intensity=75"); file.write_all(b"Led2_intensity=0"); file.write_all(b"Led3_intensity=0");},
			16..=20 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=0"); file.write_all(b"Led3_intensity=0");},
			
			21..=25 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=25"); file.write_all(b"Led3_intensity=0");},
			26..=30 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=50"); file.write_all(b"Led3_intensity=0");},
			31..=35 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=75"); file.write_all(b"Led3_intensity=0");},
			36..=40 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=100"); file.write_all(b"Led3_intensity=0");},
			
			41..=45 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=100"); file.write_all(b"Led3_intensity=25");},
			46..=50 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=100"); file.write_all(b"Led3_intensity=50");},
			51..=55 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=100"); file.write_all(b"Led3_intensity=75");},
			56..=60 => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=100"); file.write_all(b"Led3_intensity=100");},

			_ => {file.write_all(b"Led1_intensity=100"); file.write_all(b"Led2_intensity=100"); file.write_all(b"Led3_intensity=100");}
		}
        //Resetting count
        file.write_all(b"clicks=0");
		
		file_status.clear();

		// Closing files
		drop(file);

		thread::sleep(time::Duration::from_millis(10000));
    }
}
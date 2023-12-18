use std::env;

#[test]
fn env_log_level() {
    env::set_var("RUST_LOG", "debug");
    assert_eq!(logger::init(Default::default()), true);

    if cfg!(target_os = "android") {
        // android_logger does not read from environment variables
        assert_eq!(log::max_level(), log::LevelFilter::Off);
    } else {
        // env_logger reads its log level from the "RUST_LOG" environment variable
        assert_eq!(log::max_level(), log::LevelFilter::Debug);
    }
    env::remove_var("RUST_LOG");
}
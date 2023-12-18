use std::env;

#[test]
fn config_log_level() {
    // Environment variables should be overwritten by config values.
    env::set_var("RUST_LOG", "debug");

    let init_result = logger::init(
        logger::Config::default()
            .with_min_level(log::Level::Trace));

    assert_eq!(init_result, true);
    // Setting the level through the Config struct should impact both host and device
    assert_eq!(log::max_level(), log::LevelFilter::Trace);

    env::remove_var("RUST_LOG");
}
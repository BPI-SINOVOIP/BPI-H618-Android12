#[test]
fn multiple_init() {
    let first_init = logger::init(Default::default());
    let second_init = logger::init(Default::default());

    assert_eq!(first_init, true);
    assert_eq!(second_init, false);
}
#![feature(std_internals)]

use alloc::string::String;
use alloc::boxed::Box;
use core::any::Any;
use core::panic::BoxMeUp;

#[rustc_std_internal_symbol]
unsafe fn format_payload(payload: *mut &mut dyn BoxMeUp) -> String {
    let payload = Box::from_raw((*payload).take_box());
    let msg = match payload.downcast_ref::<&'static str>() {
        Some(s) => String::from(*s),
        None => match payload.downcast_ref::<String>() {
            Some(s) => String::from(s),
            None => String::from("<unsupported panic payload type>"),
        },
    };
    return msg
}

pub(crate) unsafe fn android_set_abort_message(payload: *mut &mut dyn BoxMeUp) {
        // std::ffi::CString is not available here. Use alloc::vec to create the char* argument.
        // Manually append the final null byte.
        let mut msg = format_payload(payload).into_bytes();
        msg.push(0);
        libc::android_set_abort_message(msg.as_ptr() as *const libc::c_char);
}

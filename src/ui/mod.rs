use cfg_if::cfg_if;

cfg_if! {
    if #[cfg(not(gui))] {
        mod tui;
        pub use tui::*;
    } else {
        mod gui;
        pub use gui::*;
    }
}

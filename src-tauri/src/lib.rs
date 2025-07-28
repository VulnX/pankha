pub mod util;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            util::is_root,
            util::is_ec_sys_loaded,
            util::load_ec_sys_with_write_support,
            util::get_cpu_temp,
            util::start_periodic_cpu_temp_fetcher,
            util::get_fan_rpm,
            util::set_fan_rpm,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

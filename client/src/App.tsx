import "./App.css";
import Header from "./Header.tsx";
import ControlScreen from "./ControlScreen.tsx";
import { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { message } from "@tauri-apps/plugin-dialog";
import { Window } from "@tauri-apps/api/window";

function App() {

  const hasRun = useRef(false);
  const [isECSysLoaded, setIsECSysLoaded] = useState(false);

  function stop(msg: string) {
    message(msg).then(() => {
      Window.getCurrent().close();
    });
  }

  function loadECSys() {
    invoke('load_ec_sys_with_write_support')
      .catch(e => stop(`Failed to load ec_sys : ${e}`));
  }

  function ensureECSysLoaded() {
    invoke('is_ec_sys_loaded')
      .then(loaded => {
        if (!loaded) {
          loadECSys();
        }
        setIsECSysLoaded(true);
      })
      .catch(e => stop(`Failed to check if ec_sys is loaded : ${e}`));
  }

  function ensureRoot() {
    invoke('is_root')
      .then(root => {
        if (!root) {
          stop('Please run this application as root');
        }
      });
  }

  function startPeriodicCPUTempFetcher() {
    invoke('start_periodic_cpu_temp_fetcher');
  }

  useEffect(() => {
    if (!hasRun.current) {
      ensureRoot();
      ensureECSysLoaded();
      startPeriodicCPUTempFetcher();

      hasRun.current = true;
    }
  }, []);

  return (
    <div className="app">
      <Header />
      {isECSysLoaded && <ControlScreen />}
    </div>
  );
}

export default App;

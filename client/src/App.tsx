import "./App.css";
import Header from "./Header.tsx";
import ControlScreen from "./ControlScreen.tsx";
import { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { message } from "@tauri-apps/plugin-dialog";
import { Window } from "@tauri-apps/api/window";

function App() {

  const hasRun = useRef(false);
  const [isPankhaLoaded, setIsPankhaLoaded] = useState(false);

  function stop(msg: string) {
    message(msg).then(() => {
      Window.getCurrent().close();
    });
  }

  function ensurePankhaLoaded() {
    invoke('ensure_pankha')
      .then(() => {
        setIsPankhaLoaded(true);
      })
      .catch(e => stop(`Failed to check if pankha is loaded : ${e}`));
  }

  function startPeriodicDataFetcher() {
    invoke('start_periodic_data_fetcher');
  }

  useEffect(() => {
    if (!hasRun.current) {
      ensurePankhaLoaded();
      startPeriodicDataFetcher();
      hasRun.current = true;
    }
  }, []);

  return (
    <div className="app">
      <Header />
      {isPankhaLoaded && <ControlScreen />}
    </div>
  );
}

export default App;

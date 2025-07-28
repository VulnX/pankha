import "./ControlScreen.css";
import { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { message } from "@tauri-apps/plugin-dialog";
import Slider from "./Slider";

function ControlScreen() {

    const hasRun = useRef(false);
    const [temp, setTemp] = useState<null | number>(null);
    const [rpm, setRpm] = useState(0);
    const [rpmSliderValue, setRpmSliderValue] = useState(0);
    const [sliderDisabled, setSliderDisabled] = useState(false);
    const sliderRef = useRef<null | HTMLInputElement>(null);

    useEffect(() => {
        if (!hasRun.current) {
            listen('cpu_temp', (event) => {
                setTemp(event.payload as number);
            });
            listen('fan_speed', (event) => {
                setRpm(event.payload as number);
            });
            listen('release_btn_lock', () => {
                setSliderDisabled(false);
            });

            invoke<number>('get_cpu_temp').then(setTemp);

            invoke<number>('get_fan_rpm')
                .then(rpm => {
                    setRpm(rpm);
                    setRpmSliderValue(rpm);
                });
        }
    }, []);

    function renderSlider() {
        const slider = sliderRef.current!;
        const value = Number(slider.value);
        const min = Number(slider.min);
        const max = Number(slider.max);
        const progress = (value-min) / (max-min) * 100;
        slider.style.background = 'linear-gradient(to right, var(--green) 0%, var(--green) ' + progress + '%, #fff ' + progress + '%, #fff 100%)'
    }

    useEffect(renderSlider, [rpmSliderValue]);

    function handleSliderChange(event: React.ChangeEvent<HTMLInputElement>) {
        const value = Number(event.target.value);
        setRpmSliderValue(value);
    }

    function changeRPM() {
        // Round to nearest multiple of 500
        const normalizedRPM = Math.round(rpmSliderValue / 500) * 500;
        setSliderDisabled(true);
        setRpmSliderValue(normalizedRPM);
        invoke('set_fan_rpm', { rpm: normalizedRPM })
            .catch(e => message(`Failed to change fan speed : ${e}`));
    }

    return (
        <main className="control-screen">
            <div className="circles">
                <div className="circle cpu">
                    <span>CPU<br />{temp === null ? '... ' : `${temp}° C`}
                    </span>
                </div>

                <div className="circle fan">
                    <span>FAN<br />{rpm || 'OFF'}</span>
                </div>
            </div>
            <Slider
                sliderRef={sliderRef}
                rpmSliderValue={rpmSliderValue}
                sliderDisabled={sliderDisabled}
                handleSliderChange={handleSliderChange}
                changeRPM={changeRPM}
            />
        </main>
    );
}

export default ControlScreen;
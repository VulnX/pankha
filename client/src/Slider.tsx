import { LegacyRef } from "react";
import "./Slider.css";

interface SliderProps {
    sliderRef: LegacyRef<HTMLInputElement>,
    rpmSliderValue: number;
    sliderDisabled: boolean;
    handleSliderChange: (event: React.ChangeEvent<HTMLInputElement>) => void;
    changeRPM: () => void;
}

function Slider({ sliderRef, rpmSliderValue, sliderDisabled, handleSliderChange, changeRPM }: SliderProps) {

    return (
        <div className="slider">
            <label>RPM: {rpmSliderValue}</label>
            <input
                type="range"
                min="0"
                max="5500"
                step="1"
                ref={sliderRef}
                value={rpmSliderValue}
                disabled={sliderDisabled}
                onChange={handleSliderChange}
                onMouseUp={changeRPM}
                onTouchEnd={changeRPM} />
        </div>
    );
}

export default Slider;
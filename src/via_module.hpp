#pragma once

#include "via_ui.hpp"
#include "via_virtual_module.hpp"
#include "pow2decimate.hpp"

template<int OVERSAMPLE_AMOUNT, int OVERSAMPLE_QUALITY> 
struct Via : Module {

    enum ParamIds {
        KNOB1_PARAM,
        KNOB2_PARAM,
        KNOB3_PARAM,
        A_PARAM,
        B_PARAM,
        CV2AMT_PARAM,
        CV3AMT_PARAM,
        BUTTON1_PARAM,
        BUTTON2_PARAM,
        BUTTON3_PARAM,
        BUTTON4_PARAM,
        BUTTON5_PARAM,
        BUTTON6_PARAM,
        TRIGBUTTON_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        A_INPUT,
        B_INPUT,
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,
        MAIN_LOGIC_INPUT,
        AUX_LOGIC_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MAIN_OUTPUT,
        LOGICA_OUTPUT,
        AUX_DAC_OUTPUT,
        AUX_LOGIC_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LED1_LIGHT,
        LED2_LIGHT,
        LED3_LIGHT,
        LED4_LIGHT,
        OUTPUT_GREEN_LIGHT,
        OUTPUT_RED_LIGHT,
        RED_LIGHT,
        GREEN_LIGHT,
        BLUE_LIGHT,
        PURPLE_LIGHT,
        NUM_LIGHTS
    };
    
    Via() {

        dac1DecimatorBuffer = (float*) malloc(OVERSAMPLE_AMOUNT * sizeof(float));
        dac2DecimatorBuffer = (float*) malloc(OVERSAMPLE_AMOUNT * sizeof(float));
        dac3DecimatorBuffer = (float*) malloc(OVERSAMPLE_AMOUNT * sizeof(float));

    }

    ViaModuleGeneric * virtualIO;

    uint32_t presetData[6];
    
    dsp::SchmittTrigger mainLogic;
    dsp::SchmittTrigger auxLogic;

    bool lastTrigState = false;
    bool lastAuxTrigState = false;
    
    int32_t lastTrigButton = 0;

    int32_t dacReadIndex = 0;
    int32_t adcWriteIndex = 0;
    int32_t slowIOPrescaler = 0;

    int32_t ledAState = 0;
    int32_t ledBState = 0;
    int32_t ledCState = 0;
    int32_t ledDState = 0;

    int32_t logicAState = 0;
    int32_t auxLogicState = 0;

    int32_t shAControl = 0;
    int32_t shBControl = 0;

    float shALast = 0;
    float shBLast = 0;

    float aSample = 0;
    float bSample = 0;

    int32_t clockDivider = 0;

    int32_t divideAmount = 1;

    float * dac1DecimatorBuffer;
    float * dac2DecimatorBuffer;
    float * dac3DecimatorBuffer;

    int32_t processingIndex = 0;

    #define IO_BUFFER_SIZE 4

    float aInputs[IO_BUFFER_SIZE];
    float bInputs[IO_BUFFER_SIZE];
    float mainLogicInputs[IO_BUFFER_SIZE];
    float auxLogicInputs[IO_BUFFER_SIZE];
    float cv2Inputs[IO_BUFFER_SIZE];
    float cv3Inputs[IO_BUFFER_SIZE];

    float mainOutputs[IO_BUFFER_SIZE];
    float auxOutputs[IO_BUFFER_SIZE];
    float mainLogicOutputs[IO_BUFFER_SIZE];
    float auxLogicOutputs[IO_BUFFER_SIZE];


    pow2Decimate<OVERSAMPLE_AMOUNT, OVERSAMPLE_QUALITY> dac1Decimator;
    pow2Decimate<OVERSAMPLE_AMOUNT, OVERSAMPLE_QUALITY> dac2Decimator;
    pow2Decimate<OVERSAMPLE_AMOUNT, OVERSAMPLE_QUALITY> dac3Decimator;


    // dsp::Decimator<OVERSAMPLE_AMOUNT, OVERSAMPLE_QUALITY> dac1Decimator;
    // dsp::Decimator<OVERSAMPLE_AMOUNT, OVERSAMPLE_QUALITY> dac2Decimator;
    // dsp::Decimator<OVERSAMPLE_AMOUNT, OVERSAMPLE_QUALITY> dac3Decimator;

    inline void updateSlowIO(void) {

        virtualIO->button1Input = (int32_t) params[BUTTON1_PARAM].getValue();
        virtualIO->button2Input = (int32_t) params[BUTTON2_PARAM].getValue();
        virtualIO->button3Input = (int32_t) params[BUTTON3_PARAM].getValue();
        virtualIO->button4Input = (int32_t) params[BUTTON4_PARAM].getValue();
        virtualIO->button5Input = (int32_t) params[BUTTON5_PARAM].getValue();
        virtualIO->button6Input = (int32_t) params[BUTTON6_PARAM].getValue();

        // these have a janky array ordering to correspond with the DMA stream on the hardware
        virtualIO->controls.controlRateInputs[2] = clamp((int32_t) (params[KNOB1_PARAM].getValue()), 0, 4095);
        virtualIO->controls.controlRateInputs[3] = clamp((int32_t) (params[KNOB2_PARAM].getValue()), 0, 4095);
        virtualIO->controls.controlRateInputs[1] = clamp((int32_t) (params[KNOB3_PARAM].getValue()), 0, 4095);
        // model the the 1v/oct input, scale 10.6666666 volts 12 bit adc range
        // it the gain scaling stage is inverting
        float cv1Conversion = -inputs[CV1_INPUT].getVoltage();
        // ultimately we want a volt to be a chage of 384 in the adc reading
        cv1Conversion = cv1Conversion * 384.0;
        // offset to unipolar
        cv1Conversion += 2048.0;
        // clip at rails of the input opamp
        virtualIO->controls.controlRateInputs[0] = clamp((int32_t)cv1Conversion, 0, 4095);
    }

    inline void processTriggerButton(void) {
        int32_t trigButton = clamp((int32_t)params[TRIGBUTTON_PARAM].getValue(), 0, 1);
        if (trigButton > lastTrigButton) {
            virtualIO->buttonPressedCallback();
        } else if (trigButton < lastTrigButton) {
            virtualIO->buttonReleasedCallback();
        } 
        lastTrigButton = trigButton;
    }

    // 2 sets the "GPIO" high, 1 sets it low, 0 is a no-op
    inline int32_t virtualLogicOut(int32_t logicOut, int32_t GPIO, uint32_t reg) {
        uint32_t on = (((GPIO >> (reg + 16))) & 1);
        uint32_t off = (((GPIO >> (reg))) & 1);
        return clamp(logicOut + (on * 2) - off, 0, 1);
    }

    float ledDecay = 1.f/(48000.f);

    inline void updateLEDs(void) {

        lights[LED1_LIGHT].setSmoothBrightness((float) !ledAState, ledDecay);
        lights[LED3_LIGHT].setSmoothBrightness((float) !ledBState, ledDecay);
        lights[LED2_LIGHT].setSmoothBrightness((float) !ledCState, ledDecay);
        lights[LED4_LIGHT].setSmoothBrightness((float) !ledDState, ledDecay);

        lights[RED_LIGHT].setSmoothBrightness(virtualIO->redLevelOut/4095.0, ledDecay);
        lights[GREEN_LIGHT].setSmoothBrightness(virtualIO->greenLevelOut/4095.0, ledDecay);
        lights[BLUE_LIGHT].setSmoothBrightness(virtualIO->blueLevelOut/4095.0, ledDecay);

        float output = outputs[MAIN_OUTPUT].value/8.0;
        lights[OUTPUT_RED_LIGHT].setSmoothBrightness(clamp(-output, 0.0, 1.0), ledDecay);
        lights[OUTPUT_GREEN_LIGHT].setSmoothBrightness(clamp(output, 0.0, 1.0), ledDecay);

    }

    inline void updateLogicOutputs(void) {

        ledAState = virtualLogicOut(ledAState, virtualIO->GPIOF, 7);
        ledBState = virtualLogicOut(ledBState, virtualIO->GPIOC, 14);
        ledCState = virtualLogicOut(ledCState, virtualIO->GPIOA, 2);
        ledDState = virtualLogicOut(ledDState, virtualIO->GPIOB, 2);

        logicAState = virtualLogicOut(logicAState, virtualIO->GPIOC, 13);
        auxLogicState = virtualLogicOut(auxLogicState, virtualIO->GPIOA, 12);
        shAControl = virtualLogicOut(shAControl, virtualIO->GPIOB, 8);
        shBControl = virtualLogicOut(shBControl, virtualIO->GPIOB, 9);

        virtualIO->GPIOA = 0;
        virtualIO->GPIOB = 0;
        virtualIO->GPIOC = 0;
        virtualIO->GPIOF = 0;

    }

    inline void acquireCVs(int32_t index) {
        // scale -5 - 5 V to -1 to 1 and then convert to 16 bit int;
        float cv2Scale = (32767.0 * clamp(-cv2Inputs[index]/5, -1.0, 1.0)) * params[CV2AMT_PARAM].getValue();
        float cv3Scale = (32767.0 * clamp(-cv3Inputs[index]/5, -1.0, 1.0)) * params[CV3AMT_PARAM].getValue();
        int16_t cv2Conversion = (int16_t) cv2Scale;
        int16_t cv3Conversion = (int16_t) cv3Scale;

        // no ADC buffer for now..
        virtualIO->inputs.cv2Samples[0] = cv2Conversion;
        virtualIO->inputs.cv3Samples[0] = cv3Conversion;
    }

    inline void processLogicInputs(int32_t index) {

        mainLogic.process(rescale(mainLogicInputs[index], .2, 1.2, 0.f, 1.f));
        bool trigState = mainLogic.isHigh();
        if (trigState && !lastTrigState) {
            virtualIO->mainRisingEdgeCallback();
        } else if (!trigState && lastTrigState) {
            virtualIO->mainFallingEdgeCallback();
        }
        lastTrigState = trigState; 

        auxLogic.process(rescale(auxLogicInputs[index], .2, 1.2, 0.f, 1.f));
        bool auxTrigState = auxLogic.isHigh();
        if (auxTrigState && !lastAuxTrigState) {
            virtualIO->auxRisingEdgeCallback();
        } else if (!auxTrigState && lastAuxTrigState) {
            virtualIO->auxFallingEdgeCallback();
        }
        lastAuxTrigState = auxTrigState; 

    }

    inline void updateOutputs(int32_t index) {

        int32_t samplesRemaining = OVERSAMPLE_AMOUNT;
        int32_t writeIndex = 0;

        // convert to float and downsample

        while (samplesRemaining) {

            dac1DecimatorBuffer[writeIndex] = (float) virtualIO->outputs.dac1Samples[writeIndex];
            dac2DecimatorBuffer[writeIndex] = (float) virtualIO->outputs.dac2Samples[writeIndex];
            dac3DecimatorBuffer[writeIndex] = (float) virtualIO->outputs.dac3Samples[writeIndex];

            samplesRemaining--;
            writeIndex ++;

        }

        float dac1Sample = dac1Decimator.process(dac1DecimatorBuffer);
        float dac2Sample = dac2Decimator.process(dac2DecimatorBuffer);
        float dac3Sample = dac3Decimator.process(dac3DecimatorBuffer);
        
        updateLogicOutputs();
        virtualIO->halfTransferCallback();

        // "model" the circuit
        // A and B inputs with normalled reference voltages
        float aIn = aInputs[index];
        float bIn = bInputs[index];
        
        // sample and holds
        // get a new sample on the rising edge at the sh control output
        if (shAControl > shALast) {
            aSample = aIn;
        }
        if (shBControl > shBLast) {
            bSample = bIn;
        }

        shALast = shAControl;
        shBLast = shBControl;

        // either use the sample or track depending on the sh control output
        aIn = shAControl ? aSample : aIn;
        bIn = shBControl ? bSample : bIn;

        // VCA/mixing stage
        // normalize 12 bits to 0-1
        mainOutputs[index] = (bIn*(dac2Sample/4095.0) + aIn*(dac1Sample/4095.0)); 
        auxOutputs[index] = ((dac3Sample/4095.0 - .5) * -10.666666666);
        mainLogicOutputs[index] = (logicAState * 5.0);
        auxLogicOutputs[index] = (auxLogicState * 5.0);

    }

    void updateAudioRate(void) {

        outputs[MAIN_OUTPUT].setVoltage(mainOutputs[processingIndex]); 
        outputs[AUX_DAC_OUTPUT].setVoltage(auxOutputs[processingIndex]);
        outputs[LOGICA_OUTPUT].setVoltage(mainLogicOutputs[processingIndex]);
        outputs[AUX_LOGIC_OUTPUT].setVoltage(auxLogicOutputs[processingIndex]);

        aInputs[processingIndex] = inputs[A_INPUT].isConnected() ? inputs[A_INPUT].getVoltage() : params[A_PARAM].getValue();
        bInputs[processingIndex] = (inputs[B_INPUT].isConnected() ? inputs[B_INPUT].getVoltage() : 5.0) * params[B_PARAM].getValue();
        mainLogicInputs[processingIndex] = inputs[MAIN_LOGIC_INPUT].getVoltage();
        auxLogicInputs[processingIndex] = inputs[AUX_LOGIC_INPUT].getVoltage();
        cv2Inputs[processingIndex] = inputs[CV2_INPUT].getVoltage();
        cv3Inputs[processingIndex] = inputs[CV3_INPUT].getVoltage();

        processingIndex ++;

        if (processingIndex >= IO_BUFFER_SIZE - 1) {

            for (int index = 0; index < IO_BUFFER_SIZE; index ++) {  

                acquireCVs(index);

                processLogicInputs(index);

                updateOutputs(index);

            }

            updateLEDs();

            processingIndex = 0;

        }

        clockDivider = 0;

    };


    // Parameter quantity stuff

    float reverseExpo(float expoScaled) {

        return log2(expoScaled/65536.0);

    }


    struct BScaleQuantity : ParamQuantity {

        bool bConnected(void) {

            Via * module = dynamic_cast<Via *>(this->module);

            return module->inputs[B_INPUT].isConnected();

        } 

        std::string getDisplayValueString() override {

            if (!module)
                return "";

            float v = getSmoothValue();

            if (bConnected()) {
                return string::f("%.*g", 2, v);
            } else {
                return string::f("%.*g", 2, v * 5.0);                
            }

        }

        std::string getString() override {

            if (!module)
                return "";

            if (bConnected()) {
                return getLabel() + " scale: " + getDisplayValueString();
            } else {
                return getLabel() + ": " + getDisplayValueString() + "V";                
            }

        }

        void setDisplayValueString(std::string s) override {

            float v = 0.f;
            char suffix[2];
            int n = std::sscanf(s.c_str(), "%f%1s", &v, suffix);
            if (n >= 2) {
                // Parse SI prefixes
                switch (suffix[0]) {
                    case 'n': v *= 1e-9f; break;
                    case 'u': v *= 1e-6f; break;
                    case 'm': v *= 1e-3f; break;
                    case 'k': v *= 1e3f; break;
                    case 'M': v *= 1e6f; break;
                    case 'G': v *= 1e9f; break;
                    default: break;
                }
            }
            if (n >= 1) {
                if (bConnected()) {
                    setValue(v);
                } else {
                    setValue(v/5.0);
                }
            }
        }

    };

    struct ANormalQuantity : ParamQuantity {

        std::string getDisplayValueString() override {

            float v = getSmoothValue();

            return string::f("%.*g", 2, v);                

        }

        std::string getString() override {

            if (!module)
                return "";

            Via * module = dynamic_cast<Via *>(this->module);

            bool aConnected = module->inputs[A_INPUT].isConnected();

            if (aConnected) {
                return "Overriden by A input";
            } else {
                return getLabel() + ": " + getDisplayValueString() + "V";                
            }
        }

    };

    struct ButtonQuantity : ParamQuantity {

        std::string getString() override {
            return getLabel();
        }

        void setDisplayValueString(std::string s) override {}
        
    };

    struct CV2ScaleQuantity : ParamQuantity {

        std::string getDisplayValueString() override {

            float v = getSmoothValue();

            return string::f("%.*g", 3, v);                

        }

        std::string getString() override {

            if (!module)
                return "";

            Via * module = dynamic_cast<Via *>(this->module);

            bool connected = module->inputs[CV2_INPUT].isConnected();

            if (!connected) {
                return "CV input unpatched";
            } else {
                return getLabel() + ": " + getDisplayValueString();              
            }

        }

    };

    struct CV3ScaleQuantity : ParamQuantity {

        std::string getDisplayValueString() override {

            float v = getSmoothValue();

            return string::f("%.*g", 3, v);                

        }

        std::string getString() override {

            if (!module)
                return "";

            Via * module = dynamic_cast<Via *>(this->module);

            bool connected = module->inputs[CV3_INPUT].isConnected();

            if (!connected) {
                return "CV input unpatched";
            } else {
                return getLabel() + ": " + getDisplayValueString();              
            }

        }

    };

};


/*
-----------------------------------------------------------------------------------------
~~~ todo ~~~

ToThink
    -

Hammer & Meiﬂel
    modal synth
        polyphony
        make waveform audio object
            buffer
            size
            updated
        make waveform gui object
		    if updated or resized, repaint

GUI
    reintroduce custom mousecursor
    ModalFilterSampleView
        make n use local maxima array for drawing
    BuildDate
        draw automatic normal version number
        build date in the tooltip tho
    SharedPluginState (customizable state)
        sensitivity
            drag, wheel, key
        defaultvalue
            doubleclick, key
        colourscheme
            bias not needed anymore
    Button
        makeParameter
            lacks lock feature
    TextEditor
        button that edits its label
    Editor
        makeToast (firstTimeUwU)
    Knob
        find better place for parameter lock
        make more painters
AUDIO
    FFT
        replace by 3rd party lib for performance (kiss fft or pffft)
    Oversampler
        make convolver FFT-based
        implement 4x
    sidechain
        envfol
            should appear to be added to macro
            actually sample-accurate
    MixProcessor
        code readability
        add latency compensation delay
        write cpp

-----------------------------------------------------------------------------------------

*/
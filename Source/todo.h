/*
-----------------------------------------------------------------------------------------
~~~ todo ~~~

ToThink
    -

Hammer & Mei�el
    guter Lowpass Filter f�r Resonator: SimplifiedMoog
        moduliert schnell
        hat klare Resonanz
        gain range von +12 zu +6db f�r gain match
    modal synth
        make waveform audio object
            buffer
            size
            updated
        make waveform gui object
		    if updated or resized, repaint

GUI
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
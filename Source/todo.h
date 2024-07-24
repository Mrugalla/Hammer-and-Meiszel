/*
-----------------------------------------------------------------------------------------
~~~ concepts ~~~

~~~ todo ~~~

ResonanzEnv ist noch nicht implementiert auf audio-seite
    ich glaub resonanzbreite auch nicht

vereinheitliche viele GUI aspekte um den workflow zu verbessern
    zögere nicht!

H&M Imagepflege
    sollten alle tooltips deutsch sein?

Material
    import von samples erzeugt oft keine geilen magnitudes

Param db valToStr can be -inf (gain wet [-inf, 0])
    but it can also be not (gain out [-12,12])

GUI
    reintroduce custom mousecursor
    SharedPluginState (customizable state)
        sensitivity
            drag, wheel, key
        defaultvalue
            doubleclick, key
        colourscheme
            bias not needed anymore
    all parameters currently lack visible lock feature
    TextEditor
        button that edits its label
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
#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <BinaryData.h>

#include "../Processor.h"
#include <functional>
#include <optional>

namespace gui
{
    using Colour = juce::Colour;
    using Gradient = juce::ColourGradient;
    using String = juce::String;
    using WChar = juce::juce_wchar;
    using Font = juce::Font;
	using GlyphArrangement = juce::GlyphArrangement;
    using FontOptions = juce::FontOptions;
    using Props = juce::PropertiesFile;
    using AppProps = juce::ApplicationProperties;
    using Cursor = juce::MouseCursor;
    using Image = juce::Image;
	using ScaledImage = juce::ScaledImage;
    using Graphics = juce::Graphics;
    using Mouse = juce::MouseEvent;
    using MouseWheel = juce::MouseWheelDetails;
    using Just = juce::Justification;
    using Timer = juce::Timer;
    using Path = juce::Path;
    using Point = juce::Point<int>;
    using PointF = juce::Point<float>;
    using PointD = juce::Point<double>;
    using Bounds = juce::Rectangle<int>;
    using BoundsF = juce::Rectangle<float>;
    using BoundsD = juce::Rectangle<double>;
    using Line = juce::Line<int>;
    using LineF = juce::Line<float>;
    using LineD = juce::Line<double>;
    using Image = juce::Image;
    using RectPlacement = juce::RectanglePlacement;
    using Stroke = juce::PathStrokeType;
    using Affine = juce::AffineTransform;
    using Random = juce::Random;
    using KeyPress = juce::KeyPress;
    using ValueTree = juce::ValueTree;
    using File = juce::File;
    using RangedDirectoryIterator = juce::RangedDirectoryIterator;
    using Identifier = juce::Identifier;
    using RangeF = juce::NormalisableRange<float>;
    using RangeD = juce::NormalisableRange<double>;
    using Time = juce::Time;
    using FileChooser = juce::FileChooser;
    using Component = juce::Component;
    using SystemClipboard = juce::SystemClipboard;
    using SIMD = juce::FloatVectorOperations;
    using ImageFileFormat = juce::ImageFileFormat;
    using PNGImageFormat = juce::PNGImageFormat;
    using FileOutputStream = juce::FileOutputStream;
    using Var = juce::var;
    using StringArray = juce::StringArray;
    using AudioFormatManager = juce::AudioFormatManager;
    using AudioBufferF = juce::AudioBuffer<float>;
	using RectanglePlacement = juce::RectanglePlacement;
	using ResamplingQuality = Graphics::ResamplingQuality;
	using ColourSelector = juce::ColourSelector;
	using ImageCache = juce::ImageCache;
	using Int64 = juce::int64;
	using WavAudioFormat = juce::WavAudioFormat;
	using AudioFormatWriter = juce::AudioFormatWriter;
	using MouseCursor = juce::MouseCursor;
    using FileDragAndDropTarget = juce::FileDragAndDropTarget;
	using DragAndDropTarget = juce::DragAndDropTarget;
	using DragAndDropContainer = juce::DragAndDropContainer;
    using DnDSrc = DragAndDropTarget::SourceDetails;
	using URL = juce::URL;

    using Processor = audio::Processor;

    using PID = param::PID;
    using Param = param::Param;
    using Params = param::Params;

    static constexpr float Tau = 6.28318530718f;
    static constexpr float Pi = Tau * .5f;;
    static constexpr float PiHalf = Tau * .25f;
    static constexpr float PiQuart = Tau * .125f;
	static constexpr float PiEight = Tau * .0625f;
	static constexpr float PiSixteenth = Tau * .03125f;
	static constexpr float Pi32 = Pi / 32.f;

    static constexpr int EditorWidth = 1082;
    static constexpr int EditorHeight = 722;
    static constexpr int EditorMinWidth = 100;
    static constexpr int EditorMinHeight = 100;
    static constexpr int FPS = 60;

    static constexpr double FPSD = static_cast<double>(FPS);
    static constexpr double FPSInv = 1. / FPSD;

    bool isLineBreak(WChar) noexcept;

    bool isTextCharacter(WChar) noexcept;

    namespace font
    {
        Font nel();
        Font flx();
        Font lobster();
        Font msMadi();
        Font dosisSemiBold();
        Font dosisBold();
        Font dosisExtraBold();
        Font dosisLight();
        Font dosisExtraLight();
        Font dosisMedium();
        Font dosisRegular();
        Font dosisVariable();
    }
}
#pragma once
#include "Label.h"
#include "../audio/dsp/PluginRecorder.h"

namespace gui
{
	struct LabelPluginRecorder :
		public Label,
		public juce::DragAndDropContainer
	{
		using Recorder = dsp::PluginRecorder;
		using DnDTarget = juce::DragAndDropTarget;
		using DnDSrc = DnDTarget::SourceDetails;

		LabelPluginRecorder(Utils& u) :
			Label(u),
			recorder(u.audioProcessor.recorder),
			dragImage(Image(Image::ARGB, 20, 20, true))//,
			//scaledImage()
		{
			setInterceptsMouseClicks(true, true);

			Path path;
			path.startNewSubPath(10, 0);
			path.quadraticTo(10, 10, 20, 10);
			path.quadraticTo(10, 10, 10, 20);
			path.quadraticTo(10, 10, 0, 10);
			path.quadraticTo(10, 10, 10, 0);
			const Stroke stroke(2.f, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
			Graphics g{ dragImage };
			setCol(g, CID::Hover);
			g.strokePath(path, stroke);
			//scaledImage = ScaledImage(dragImage, 1.);
		}

		void mouseDown(const Mouse&) override
		{
			const auto file = saveWav();
			if (!file.existsAsFile())
				return;
			StringArray files;
			const bool canMoveFiles = true;
			files.add(file.getFullPathName());
			performExternalDragDropOfFiles
			(
				files,
				canMoveFiles,
				this,
				[]() {}
			);
			const Var sourceDescription(420);
			const Point imageOffsetFromMouse(-10, -10);
			//ScaledImage scaledImage(dragImage);
			startDragging(sourceDescription, this, dragImage, true, &imageOffsetFromMouse, nullptr);
		}

	private:
		Recorder& recorder;
		Image dragImage;
		//ScaledImage scaledImage;

		File getTheFile()
		{
			const auto& user = *utils.audioProcessor.state.props.getUserSettings();
			const auto settingsFile = user.getFile();
			const auto userDirectory = settingsFile.getParentDirectory();
			const auto file = userDirectory.getChildFile("HnM.wav");
			return file;
		}

		void saveWav(const dsp::AudioBufferF& buffer)
		{
			const auto file = getTheFile();
			if(file.existsAsFile())
				file.deleteFile();
			file.create();
			WavAudioFormat format;
			std::unique_ptr<AudioFormatWriter> writer;
			const auto numChannels = utils.audioProcessor.getBus(true, 0)->getNumberOfChannels();
			const auto Fs = utils.audioProcessor.getSampleRate();
			writer.reset(format.createWriterFor(new FileOutputStream(file),
				Fs,
				numChannels,
				24,
				{},
				0
			));
			const auto numSamples = buffer.getNumSamples();
			if (!writer)
				return;
			writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
		}

		File saveWav()
		{
			utils.audioProcessor.suspendProcessing(true);
			const auto& recording = recorder.getRecording();
			utils.audioProcessor.suspendProcessing(false);
			saveWav(recording);
			return getTheFile();
		}
	};
}
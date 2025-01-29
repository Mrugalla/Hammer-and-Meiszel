#include "LabelPluginRecorder.h"

namespace gui
{

	LabelPluginRecorder::LabelPluginRecorder(Utils& u) :
		Label(u),
		recorder(u.audioProcessor.recorder),
		dragImage(Image::ARGB, 20, 20, true),
		scaledImage(dragImage),
		file()
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
	}

	void LabelPluginRecorder::mouseDown(const Mouse& mouse)
	{
		saveWav();
		const Var sourceDescription("pluginrecorder");
		Point imageOffsetFromMouse(0, 0);
		if (file.existsAsFile())
			startDragging
			(
				sourceDescription,
				this,
				scaledImage,
				true,
				&imageOffsetFromMouse,
				&mouse.source
			);
	}

	bool LabelPluginRecorder::shouldDropFilesWhenDraggedExternally(const DnDSrc&,
		StringArray& files, bool& canMoveFiles)
	{
		files.clearQuick();
		files.add(file.getFullPathName());
		canMoveFiles = false;
		return true;
	}

	void LabelPluginRecorder::getTheFile()
	{
		const auto& user = *utils.audioProcessor.state.props.getUserSettings();
		const auto settingsFile = user.getFile();
		const auto userDirectory = settingsFile.getParentDirectory();
		file = userDirectory.getChildFile("HnM.wav");
	}

	bool LabelPluginRecorder::saveWav(const dsp::AudioBufferF& buffer)
	{
		getTheFile();
		if (file.existsAsFile())
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
			return false;
		writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
		return true;
	}

	void LabelPluginRecorder::saveWav()
	{
		utils.audioProcessor.suspendProcessing(true);
		const auto& recording = recorder.getRecording();
		utils.audioProcessor.suspendProcessing(false);
		const auto success = saveWav(recording);
		if (success)
			return getTheFile();
		file = File();
	}
}
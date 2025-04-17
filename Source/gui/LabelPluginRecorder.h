#pragma once
#include "Label.h"
#include "../audio/dsp/PluginRecorder.h"

namespace gui
{
	struct LabelPluginRecorder :
		public Label,
		public DragAndDropContainer
	{
		using Recorder = dsp::PluginRecorder;
		
		LabelPluginRecorder(Utils&);

		void mouseDown(const Mouse&) override;

		// dndSrc, files, canMoveFiles
		bool shouldDropFilesWhenDraggedExternally(const DnDSrc&,
			StringArray&, bool&) override;

	private:
		Recorder& recorder;
		Image dragImage;
		ScaledImage scaledImage;
		File file;

		void getTheFile();

		bool saveWav(const dsp::AudioBufferF&);

		void saveWav();
	};



}
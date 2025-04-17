#pragma once
#include "Label.h"
#include "../audio/dsp/PluginRecorder.h"

namespace gui
{
	struct LabelPluginRecorder :
		public Label,
		public DragAndDropContainer
	{
		enum { kHoverAni, kNumAnis };

		struct ImgData
		{
			Image getImage();

			const void* data;
			int size;
		};

		using Recorder = dsp::PluginRecorder;
		
		LabelPluginRecorder(Utils&);

		void mouseEnter(const Mouse&) override;

		void mouseExit(const Mouse&) override;

		void mouseDown(const Mouse&) override;

		// dndSrc, files, canMoveFiles
		bool shouldDropFilesWhenDraggedExternally(const DnDSrc&,
			StringArray&, bool&) override;

		void setImage(const void*, int);

		void updateImage();

		void paint(Graphics& g) override;
	private:
		Image bgImg;
		Recorder& recorder;
		Image dragImage;
		ScaledImage scaledImage;
		File file;
		ImgData imgData;

		void getTheFile();

		bool saveWav(const dsp::AudioBufferF&);

		void saveWav();
	};



}
#pragma once
#include <JuceHeader.h>
#include "audio/Using.h"
#include <array>
#include <chrono>

namespace test
{
	struct MIDIRandomMelodyGenerator
	{
		static constexpr float Likelyness = .001f;
		static constexpr int NoteMin = 36;
		static constexpr int NoteMax = 96;
		static constexpr int NoteRange = NoteMax - NoteMin;

		struct NoteInfo
		{
			NoteInfo() :
				note(NoteMin),
				noteOn(false)
			{}

			int note;
			bool noteOn;
		};

		struct NoteStack
		{
			NoteStack() :
				stack(),
				idx(-1)
			{}

			bool isFull() const noexcept
			{
				for (const auto& note : stack)
					if (!note.noteOn)
						return false;
				return true;
			}

			bool isEmpty() const noexcept
			{
				for (const auto& note : stack)
					if (note.noteOn)
						return false;
				return true;

			}

			void sendNoteOff(juce::MidiBuffer& buffer, juce::Random& rand, int s) noexcept
			{
				while (true)
				{
					const auto i = rand.nextInt(4);
					if (stack[i].noteOn)
					{
						buffer.addEvent(juce::MidiMessage::noteOff(1, stack[i].note), s);
						stack[i].noteOn = false;
						return;
					}
				}
				
			}

			void sendNoteOn(juce::MidiBuffer& buffer, juce::Random& rand, int s) noexcept
			{
				const auto noteNumber = NoteMin + rand.nextInt(NoteRange);
				buffer.addEvent(juce::MidiMessage::noteOn(1, noteNumber, juce::uint8(100)), s);
				idx = (idx + 1) % stack.size();
				stack[idx].noteOn = true;
				stack[idx].note = noteNumber;
			}

			const NoteInfo& operator[](int i) const noexcept
			{
				return stack[i];
			}

			std::array<NoteInfo, 4> stack;
			int idx;
		};

		MIDIRandomMelodyGenerator() :
			rand(),
			buffer(),
			stack()
		{}

		void operator()(juce::MidiBuffer& midi, int numSamples) noexcept
		{
			buffer.clear();
			buffer.addEvents(midi, 0, numSamples, 0);

			const bool eventOccurs = rand.nextFloat() < Likelyness;
			if (eventOccurs)
			{
				const auto s = rand.nextInt(numSamples);
				if (stack.isFull())
					stack.sendNoteOff(buffer, rand, s);
				else if (stack.isEmpty())
					stack.sendNoteOn(buffer, rand, s);
				else
				{
					const bool noteOnOrOff = rand.nextBool();
					if (noteOnOrOff)
						stack.sendNoteOn(buffer, rand, s);
					else
						stack.sendNoteOff(buffer, rand, s);
				}
			}

			midi.swapWith(buffer);
		}

		juce::Random rand;
		juce::MidiBuffer buffer;
		NoteStack stack;
	};

	struct SpeedTestPB
	{
		using PBFunc = std::function<void(double**, juce::MidiBuffer&, int, int)>;

		struct SpeedTest
		{
			SpeedTest() :
				startTime(std::chrono::high_resolution_clock::now())
			{}

			std::chrono::nanoseconds getElapsed()
			{
				const auto endTime = std::chrono::high_resolution_clock::now();
				return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
			}

			std::chrono::high_resolution_clock::time_point startTime;
		};

		SpeedTestPB(PBFunc&& pbFunc, int iterations, juce::String&& fileName) :
			buffer(2, dsp::BlockSize2x),
			midi(),
			rand()
		{
			auto block = buffer.getArrayOfWritePointers();
			double* samples[] = { block[0], block[1] };

			std::vector<std::chrono::nanoseconds> times;

			for(auto i = 0; i < iterations; ++i)
			{
				for (auto ch = 0; ch < 2; ++ch)
					for (auto s = 0; s < dsp::BlockSize2x; ++s)
						samples[ch][s] = rand.nextDouble();

				SpeedTest test;
				pbFunc(samples, midi, 2, dsp::BlockSize2x);
				times.push_back(test.getElapsed());
			}

			auto average = times[0];
			for (auto i = 1; i < times.size(); ++i)
				average += times[i];
			average /= times.size();

			auto max = times[0];
			for (auto i = 1; i < times.size(); ++i)
				if (times[i] > max)
					max = times[i];

			auto sortedTimes = times;
			std::sort(sortedTimes.begin(), sortedTimes.end());
			auto median = sortedTimes[sortedTimes.size() / 2];

			auto desktop = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
			auto folder = desktop.getChildFile("SpeedTests");
			if (!folder.exists())
				folder.createDirectory();
			auto fileNameFull = juce::String(__DATE__) + juce::String(__TIME__) + " " + fileName + ".txt";
			fileNameFull = juce::File::createLegalFileName(fileNameFull);
			auto file = folder.getChildFile(fileNameFull);
			if (file.existsAsFile())
				file.deleteFile();
			file.create();

			file.appendText("Iterations: " + juce::String(iterations) + "\n");
			file.appendText("Average: " + juce::String(average.count() / 1000000.) + "ms\n");
			file.appendText("Median: " + juce::String(median.count() / 1000000.) + "ms\n");
			file.appendText("Worse Case: " + juce::String(max.count() / 1000000.) + "ms\n");
			for (auto& time : times)
				file.appendText(juce::String(time.count() / 1000000.) + "ms\n");
		}

		dsp::AudioBuffer buffer;
		dsp::MidiBuffer midi;
		juce::Random rand;
	};
}
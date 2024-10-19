#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace arch
{
	using String = juce::String;
	using ValueTree = juce::ValueTree;
	using Var = juce::var;
	using MemoryBlock = juce::MemoryBlock;
	using File = juce::File;
	using XML = std::unique_ptr<juce::XmlElement>;
	using XMLDoc = juce::XmlDocument;
	using Proc = juce::AudioProcessor;
	using Props = juce::ApplicationProperties;
	
	struct State
	{
		State();

		State(const String&);

		void savePatch(const Proc&, MemoryBlock&) const;

		void savePatch(const File&) const;

		void loadPatch(const XML&);

		/* processor, data, sizeInBytes */
		void loadPatch(const Proc&, const void*, int);

		/* data, sizeInBytes */
		void loadPatch(const char*, int );

		void loadPatch(const File&);

		void loadPatch(const ValueTree&);
		
		// path, var; paths like "params/param/gain/value"
		void set(const String&, Var&&);

		// paths like "params/param/gain/value"
		const Var* get(const String&) const;
		
		ValueTree state;
		Props props;
	};
}
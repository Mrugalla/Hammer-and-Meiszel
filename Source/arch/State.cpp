#include "State.h"

namespace arch
{
	void makePropsOptions(Props& props)
	{
		juce::PropertiesFile::Options options;
		options.applicationName = JucePlugin_Name;
		options.filenameSuffix = ".settings";
		options.folderName = JucePlugin_Manufacturer + juce::File::getSeparatorString() + JucePlugin_Name;
		options.osxLibrarySubFolder = "Application Support";
		options.commonToAllUsers = false;
		options.ignoreCaseOfKeyNames = true;
		options.doNotSave = false;
		options.millisecondsBeforeSaving = 100;
		options.storageFormat = juce::PropertiesFile::storeAsXML;

		props.setStorageParameters(options);
	}

	State::State() :
		state("state"),
		props()
	{
		makePropsOptions(props);
	}

	State::State(const String& str) :
		state(state.fromXml(str)),
		props()
	{
		makePropsOptions(props);
	}

	void State::savePatch(const Proc& p, MemoryBlock& destData) const
	{
		std::unique_ptr<juce::XmlElement> xml(state.createXml());
		p.copyXmlToBinary(*xml, destData);
	}

	void State::savePatch(const File& xmlFile) const
	{
		if (!xmlFile.hasFileExtension(".xml")) return;
		if (xmlFile.existsAsFile())
			xmlFile.deleteFile();
		xmlFile.create();
		xmlFile.appendText(state.toXmlString());
	}

	void State::loadPatch(const XML& xmlState)
	{
		if (xmlState.get() != nullptr)
			if (xmlState->hasTagName(state.getType()))
				loadPatch(juce::ValueTree::fromXml(*xmlState));
	}

	void State::loadPatch(const Proc& p, const void* data, int sizeInBytes)
	{
		loadPatch(p.getXmlFromBinary(data, sizeInBytes));
	}

	void State::loadPatch(const char* data, int sizeInBytes)
	{
		loadPatch(juce::XmlDocument::parse(String(data, sizeInBytes)));
	}

	void State::loadPatch(const File& xmlFile)
	{
		if (!xmlFile.hasFileExtension(".xml")) return;
		if (!xmlFile.existsAsFile()) return;
		const auto xml = XMLDoc::parse(xmlFile);
		if (xml == nullptr) return;
		if (!xml->hasTagName(state.getType())) return;
		loadPatch(ValueTree::fromXml(*xml));
	}

	void State::loadPatch(const ValueTree& vt)
	{
		state = vt;
	}

	using StringArray = juce::StringArray;

	void State::set(const String& path, Var&& var)
	{
		StringArray tokens;
		tokens.addTokens(path, "/", "\"");

		const auto lastToken = tokens.size() - 1;
		
		auto child = state;
		
		for (auto t = 0; t < tokens.size(); ++t)
		{
			if (t == lastToken)
			{
				child.setProperty(tokens[t], var, nullptr);
				return;
			}

			auto nChild = child.getChildWithName(tokens[t]);
			if (!nChild.isValid())
			{
				nChild = ValueTree(tokens[t]);
				child.appendChild(nChild, nullptr);
			}
			child = nChild;
		}
	}

	const Var* State::get(const String& path) const
	{
		StringArray tokens;
		tokens.addTokens(path, "/", "\"");
		
		const auto lastToken = tokens.size() - 1;

		auto child = state;

		for (auto t = 0; t < tokens.size(); ++t)
		{
			if (t == lastToken)
				return child.getPropertyPointer(tokens[t]);

			auto nChild = child.getChildWithName(tokens[t]);
			if (!nChild.isValid())
				return nullptr;
			child = nChild;
		}
		return nullptr;
	}

	// set and get for global state
}
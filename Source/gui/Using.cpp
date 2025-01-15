#include "Using.h"

namespace gui
{
	bool isLineBreak(WChar wChar) noexcept
	{
		return wChar == '\n' || wChar == '\r' || wChar == '\r\n';
	}

	bool isTextCharacter(WChar wchar) noexcept
	{
		return wchar >= 0x20 && wchar <= 0x7E;
	}

	namespace font
	{
		Font getFont(const char* ttf, size_t size)
		{
			const auto typeface = juce::Typeface::createSystemTypefaceFor(ttf, size);
			const FontOptions fontOptions(typeface);
			return Font(fontOptions);
		}

		Font nel()
		{
			return getFont(BinaryData::nel19_ttf, BinaryData::nel19_ttfSize);
		}

		Font flx()
		{
			return getFont(BinaryData::felixhand_02_ttf, BinaryData::felixhand_02_ttfSize);
		}

		Font lobster()
		{
			return getFont(BinaryData::LobsterRegular_ttf, BinaryData::LobsterRegular_ttfSize);
		}

		Font msMadi()
		{
			return getFont(BinaryData::MsMadiRegular_ttf, BinaryData::MsMadiRegular_ttfSize);
		}

		Font dosisSemiBold()
		{
			return getFont(BinaryData::DosisSemiBold_ttf, BinaryData::DosisSemiBold_ttfSize);
		}

		Font dosisBold()
		{
			return getFont(BinaryData::DosisBold_ttf, BinaryData::DosisBold_ttfSize);
		}

		Font dosisExtraBold()
		{
			return getFont(BinaryData::DosisExtraBold_ttf, BinaryData::DosisExtraBold_ttfSize);
		}

		Font dosisLight()
		{
			return getFont(BinaryData::DosisLight_ttf, BinaryData::DosisLight_ttfSize);
		}

		Font dosisExtraLight()
		{
			return getFont(BinaryData::DosisExtraLight_ttf, BinaryData::DosisExtraLight_ttfSize);
		}

		Font dosisMedium()
		{
			return getFont(BinaryData::DosisMedium_ttf, BinaryData::DosisMedium_ttfSize);
		}

		Font dosisRegular()
		{
			return getFont(BinaryData::DosisRegular_ttf, BinaryData::DosisRegular_ttfSize);
		}

		Font dosisVariable()
		{
			return getFont(BinaryData::DosisVariableFont_wght_ttf, BinaryData::DosisVariableFont_wght_ttfSize);
		}
	}
}
#include "Utils.h"

namespace gui
{
	Utils::Utils(Component& _pluginTop, Processor& _audioProcessor) :
		eventSystem(),
		callbacks(),
		pluginTop(_pluginTop),
		audioProcessor(_audioProcessor),
		params(audioProcessor.params),
		thicc(2.f)
	{
		Colours::c.init(audioProcessor.state.props.getUserSettings());
	}

	void Utils::add(Callback* ncb)
	{
		callbacks.add(ncb);
	}

	void Utils::remove(Callback* ncb)
	{
		callbacks.remove(ncb);
	}

	std::vector<Param*>& Utils::getAllParams() noexcept
	{
		return params.data();
	}

	const std::vector<Param*>& Utils::getAllParams() const noexcept
	{
		return params.data();
	}

	Param& Utils::getParam(PID pID) noexcept
	{
		return params(pID);
	}

	const Param& Utils::getParam(PID pID) const noexcept
	{
		return params(pID);
	}

	ValueTree Utils::getState() const noexcept
	{
		return audioProcessor.state.state;
	}

	void Utils::resized()
	{
		auto a = std::min(pluginTop.getWidth(), pluginTop.getHeight());
		auto t = static_cast<float>(a) * .0035f;
		thicc = t < 1.f ? 1.f : t;
	}

	float Utils::getDragSpeed() const noexcept
	{
		const auto height = static_cast<float>(pluginTop.getHeight());
		const auto speed = DragSpeed * height;
		return speed;
	}

	float Utils::fontHeight() const noexcept
	{
		const auto w = static_cast<float>(pluginTop.getWidth());
		const auto h = static_cast<float>(pluginTop.getHeight());

		const auto avr = (w + h) * .5f;
		const auto norm = (avr - 500.f) / 500.f;
		return std::floor(8.5f + norm * 5.f);
	}

	void Utils::giveDAWKeyboardFocus()
	{
		pluginTop.giveAwayKeyboardFocus();
	}

	ValueTree Utils::savePatch()
	{
		audioProcessor.pluginProcessor.savePatch(audioProcessor.state);
		return audioProcessor.state.state;
	}

	void Utils::loadPatch(const ValueTree& vt)
	{
		audioProcessor.state.loadPatch(vt);
		audioProcessor.pluginProcessor.loadPatch(audioProcessor.state);
	}

	Props& Utils::getProps() noexcept
	{
		return *audioProcessor.state.props.getUserSettings();
	}

	Point Utils::getScreenPosition() const noexcept
	{
		return pluginTop.getScreenPosition();
	}
}
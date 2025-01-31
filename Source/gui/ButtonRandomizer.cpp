#include "ButtonRandomizer.h"

namespace gui
{
    ButtonRandomizer::ButtonRandomizer(Utils& u, String&& id) :
        Button(u),
        randomizables(),
        randFuncs(),
        randomizer(*u.audioProcessor.state.props.getUserSettings(), std::move(id))
    {
        const auto op = makeButtonOnPaint(false, getColour(CID::Bg));

        makePaintButton
        (
            *this,
            [op](Graphics& g, const Button& b)
            {
                op(g, b);

                const auto& utils = b.utils;
                const auto thicc = utils.thicc;
                const auto thicc2 = thicc * 2.f;

                const auto hoverPhase = b.callbacks[Button::kHoverAniCB].phase;
                const auto clickPhase = b.callbacks[Button::kClickAniCB].phase;

                const auto lineThiccness = thicc + clickPhase * (thicc2 - thicc);
                const auto margin = thicc2 - lineThiccness - hoverPhase * thicc;
                const auto bounds = maxQuadIn(b.getLocalBounds().toFloat()).reduced(margin);

                const auto iW = bounds.getWidth() / 3.f;
				const auto iH = bounds.getHeight() / 2.f;
				auto iX = bounds.getX();
				
                setCol(g, CID::Interact);
                for (auto i = 0; i < 3; ++i)
                {
                    auto iY = bounds.getY();
                    for (auto j = 0; j < 2; ++j)
                    {
						const auto area = maxQuadIn(BoundsF(iX, iY, iW, iH)).reduced(lineThiccness);
						g.fillEllipse(area);
						iY += iH;
                    }
                    iX += iW;
                }
            },
            makeTooltip()
        );

		onClick = [&](const Mouse& mouse)
        {
            operator()(mouse.mods.isLeftButtonDown(), !mouse.mods.isShiftDown());
        };
    }

	void ButtonRandomizer::add(PID p)
	{
		add(utils.audioProcessor.params[p]);
	}

    void ButtonRandomizer::add(Param* p)
    {
        randomizables.push_back(p);
    }

    void ButtonRandomizer::add(std::vector<Param*>&& p)
    {
        randomizables = p;
    }

    void ButtonRandomizer::add(const std::vector<Param*>& p)
    {
        randomizables = p;
    }

    void ButtonRandomizer::add(const RandFunc& p)
    {
        randFuncs.push_back(p);
    }

    void ButtonRandomizer::operator()(bool seedUp, bool isAbsolute)
    {
        if(isAbsolute)
            for (auto randomizable : randomizables)
            {
                randomizable->setModDepth(0.f);
            }
        randomizer.updateSeed(seedUp);

        for (auto randomizable : randomizables)
        {
            const auto pID = randomizable->id;
            if (pID != PID::Power && pID != PID::SoftClip && pID != PID::GainWet &&
                pID != PID::GainOut && pID != PID::Mix && pID != PID::Macro &&
                pID != PID::XenSnap && pID != PID::StereoConfig)
            {
                const auto& range = randomizable->range;

                float valY, mdY, bsY;
                if (isAbsolute)
                {
                    const auto v = randomizer();
                    const auto vD = range.convertFrom0to1(v);
                    const auto vL = range.snapToLegalValue(vD);
                    valY = range.convertTo0to1(vL);
					const bool up = randomizer() > .5f;
                    if (up)
						mdY = randomizer() * (1.f - valY);
                    else
						mdY = randomizer() * -valY;
                    bsY = randomizer();
                }
                else
                {
                    const auto valNorm = randomizable->getValue();
                    auto valRand = randomizer() * .05f - .025f;
                    valY = juce::jlimit(0.f, 1.f, valNorm + valRand);
                    const auto md = randomizable->getModDepth();
                    const auto bias = randomizable->getModBias();
                    if (md != 0.f)
                    {
                        valRand = randomizer() * .05f - .025f;
                        mdY = juce::jlimit(-1.f, 1.f, md + valRand);
                        valRand = randomizer() * .05f - .025f;
						bsY = juce::jlimit(0.f, 1.f, bias + valRand);
					}
                    else
                    {
                        mdY = md;
						bsY = bias;
                    }
                }
                randomizable->setValueWithGesture(valY);
                randomizable->setModDepth(mdY);
				randomizable->setModBias(bsY);
            }
        }
        for (auto& func : randFuncs)
            func(randomizer);

        setTooltip(makeTooltip());
    }

    void ButtonRandomizer::mouseEnter(const Mouse& mouse)
    {
        setTooltip(makeTooltip());
        Button::mouseEnter(mouse);
    }

    String ButtonRandomizer::makeTooltip()
    {
        Random rand;
        static constexpr float count = 267.f;
        const auto v = static_cast<int>(std::round(rand.nextFloat() * count));
        switch (v)
        {
        case 0: return "Do it!";
        case 1: return "Don't you dare it!";
        case 2: return "But... what if it goes wrong???";
        case 3: return "Nature is random too, so this is basically analog, right?";
        case 4: return "Life is all about exploration..";
        case 5: return "What if I don't even exist?";
        case 6: return "Idk, it's all up to you.";
        case 7: return "This randomizes the parameter values. Yeah..";
        case 8: return "Born too early to explore space, born just in time to hit the randomizer.";
        case 9: return "Imagine someone sitting there writing down all these phrases.";
        case 10: return "Will this save your snare from sucking ass?";
        case 11: return "Producer-san >.< d.. don't tickle me there!~";
        case 12: return "I mean, whatever.";
        case 13: return "Never commit. Just dream!";
        case 14: return "I wonder, what will happen if I...";
        case 15: return "Hit it for the digital warmth.";
        case 16: return "Don't you love cats? They are so cute :3";
        case 17: return "We should collab some time, bro.";
        case 18: return "Did you just hover the button to see what's in here this time?";
        case 19: return "It's not just a phase!";
        case 20: return "No time for figuring out parameter values manually, huh?";
        case 21: return "My cat is meowing at the door because there is a mouse rn lol";
        case 22: return "Yeeeaaaaahhhh!!!! :)";
        case 23: return "Ur hacked now >:) no just kidding ^.^";
        case 24: return "What would you do if your computer could handle 1million phasers?";
        case 25: return "It's " + (juce::Time::getCurrentTime().getHours() < 10 ? juce::String("0") + static_cast<juce::String>(juce::Time::getCurrentTime().getHours()) : static_cast<juce::String>(juce::Time::getCurrentTime().getHours())) + ":" + (juce::Time::getCurrentTime().getMinutes() < 10 ? juce::String("0") + static_cast<juce::String>(juce::Time::getCurrentTime().getMinutes()) : static_cast<juce::String>(juce::Time::getCurrentTime().getMinutes())) + " o'clock now.";
        case 26: return "I once was a beat maker, too, but then I took a compressor to the knee.";
        case 27: return "It's worth a try.";
        case 28: return "Omg, your music is awesome dude. Keep it up!";
        case 29: return "I wish there was an anime about music producers.";
        case 30: return "Days are too short, but I also don't want gravity to get heavier.";
        case 31: return "Yo, let's order some pizza!";
        case 32: return "I wanna be the very best, like no one ever was!!";
        case 33: return "Hm... yeah, that could be cool.";
        case 34: return "Maybe...";
        case 35: return "Well.. perhaps.";
        case 36: return "Here we go again.";
        case 37: return "What is the certainty of a certainty to mean a certain certainty?";
        case 38: return "One of my favorite cars is the RX7 so i found it quite funny when Izotope released that plugin.";
        case 39: return "Do you know Echobode? It's one of my favourite plugins!";
        case 40: return "Have you ever tried to make eurobeat? It's really hard for some reason.";
        case 41: return "Wanna lose control?";
        case 42: return "Do you have any more of dem randomness pls?";
        case 43: return "How random do you want it to be, sir? Yes.";
        case 44: return "Programming is not creative. I am a computer.";
        case 45: return "We should all be more mindful to each other.";
        case 46: return "Next-Level AI will randomize ur parameters!";
        case 47: return "All The Award-Winning Audio-Engineers Use This Button!!";
        case 48: return "The fact that you can't undo it only makes it better.";
        case 49: return "When things are almost as fast as light, reality bends.";
        case 50: return "I actually come from the future. Don't tell anyone pls!";
        case 51: return "You're mad!";
        case 52: return "Your ad could be here! ;)";
        case 53: return "What colour-Scheme does your tune sound like?";
        case 54: return "I wish Dyson Spheres existed already!";
        case 55: return "This is going to be so cool! OMG";
        case 56: return "Plants. There should be more of them.";
        case 57: return "10 Vibrato Mistakes Every Noob Makes: No. 7 Will Make U Give Up On Music!";
        case 58: return "Yes, I'll add more of these some other time.";
        case 59: return "The world wasn't ready for No Man's Sky. That's all.";
        case 60: return "Temposynced Tremolos are not Sidechain Compressors.";
        case 61: return "I can't even!";
        case 62: return "Let's drift off into the distance together..";
        case 63: return "When I started making NEL I wanted to make a tape emulation.";
        case 64: return "Scientists still trying to figure this one out..";
        case 65: return "Would you recommend this button to your friends?";
        case 66: return "This is a very bad feature. Don't use it!";
        case 67: return "I don't know what to say about this button..";
        case 68: return "A parallel universe, in which you will use this button now, exists.";
        case 69: return "This is actually message no. 69, haha";
        case 70: return "Who needs control anyway?";
        case 71: return "I have the feeling this time it will turn out really cool!";
        case 72: return "This turns all parameters up right to 11.";
        case 73: return "Tranquilize Your Music. Idk why, but it sounds cool.";
        case 74: return "I'm indecisive...";
        case 75: return "That's a good idea!";
        case 76: return "Once upon a time there was a traveller who clicked this button..";
        case 77: return "10/10 Best Decision!";
        case 78: return "Beware! Only really skilled audio professionals use this feature.";
        case 79: return "What would be your melody's name if it was a human being?";
        case 80: return "What if humanity was just a failed experiment by a higher species?";
        case 81: return "Enter the black hole to stop time!";
        case 82: return "Did you remember to water your plants yet?";
        case 83: return "I'm just a simple button. Nothing special to see here.";
        case 84: return "You're using this plugin. That makes you a cool person.";
        case 85: return "Only the greatest DSP technology was used in this parameter randomizer!";
        case 86: return "I am not fun at parties indeed.";
        case 87: return "This button makes it worse!";
        case 88: return "I am not sure what this is going to do.";
        case 89: return "If your music was a mountain, what shape would it be like?";
        case 90: return "NEL is the best vibrato plugin in the world. Tell all ur friends!";
        case 91: return "Do you feel the vibrations?";
        case 92: return "Defrost or Reheat? You decide!";
        case 93: return "Don't forget to hydrate yourself, king/queen.";
        case 94: return "How long does it take to get to the next planet at this speed?";
        case 95: return "What if there is a huge wall around the whole universe?";
        case 96: return "Controlled loss of control. So basically drifting! Yeah!";
        case 97: return "I talk to the wind. My words are all carried away.";
        case 98: return "Captain, we need to warp now! There is no time.";
        case 99: return "Where are we now?";
        case 100: return "Randomize me harder, daddy!~";
        case 101: return "Drama!";
        case 102: return "Turn it up! Well, this is not a knob, but you know, it's cool.";
        case 103: return "You like it dangerous, huh?";
        case 104: return "QUICK! We are under attack!";
        case 105: return "Yes, you want this!";
        case 106: return "The randomizer is better than your presets!";
        case 107: return "Are you a decide-fan, or a random-enjoyer?";
        case 108: return "Let's get it started! :D";
        case 109: return "Do what you have to do...";
        case 110: return "This is a special strain of random. ;)";
        case 111: return "Return to the battlefield or get killed.";
        case 112: return "~<* Easy Peazy Lemon Squeezy *>~";
        case 113: return "Why does it sound like dubstep?";
        case 114: return "Excuse me.. Have you seen my sanity?";
        case 115: return "In case of an emergency, push the button!";
        case 116: return "Based.";
        case 117: return "Life is a series of random collisions.";
        case 118: return "It is actually possible to add too much salt to spaghetti.";
        case 119: return "You can't go wrong with random, except when you do.";
        case 120: return "I have not implemented undo yet, but you like to live dangerously :)";
        case 121: return "404 - Creative message not found. Contact our support pls.";
        case 122: return "Press jump twice to perform a doub.. oh wait, wrong app.";
        case 123: return "And now for the ultimate configuration!";
        case 124: return "Subscribe for more random messages! (Only 42$/mon)";
        case 125: return "I love you <3";
        case 126: return "Me? Well...";
        case 127: return "What happens if I press this?";
        case 128: return "Artificial Intelligence! Not used here, but it sounds cool.";
        case 129: return "My internet just broke so why not just write another msg in here, right?";
        case 130: return "Mood.";
        case 131: return "I'm only a randomizer, after all...";
        case 132: return "There is a strong correlation between you and awesomeness.";
        case 133: return "Yes! Yes! Yes!";
        case 134: return "Up for a surprise?";
        case 135: return "This is not a plugin. It is electricity arranged swag-wise.";
        case 136: return "Chairs do not exist.";
        case 137: return "There are giant spiders all over my house and I have no idea what to do :<";
        case 138: return "My cat is lying on my lap purring and she's so cute omg!!";
        case 139: return "I come here and add more text whenever I procrastinate from fixing bugs.";
        case 140: return "Meow :3";
        case 141: return "N.. Nyan? uwu";
        case 142: return "Let's Go!";
        case 143: return "Never Gonna Let You Down! Never Gonna Give You Up! sry..";
        case 144: return "Push It!";
        case 145: return "Do You Feel The NRG??";
        case 146: return "We could escape the great filter if we only vibed stronger..";
        case 147: return "Check The Clock. It's time for randomization.";
        case 148: return "The first version of NEL was released in 2019.";
        case 149: return "My first plugin NEL was named after my son, Lionel.";
        case 150: return "If this plugin breaks, it's because your beat is too fire!";
        case 151: return "Go for it!";
        case 152: return "<!> nullptr exception: please contact the developer. <!>";
        case 153: return "Wild Missingno. appeared!";
        case 154: return "Do you have cats? Because I love cats. :3";
        case 155: return "There will be a netflix adaption of this plugin soon.";
        case 156: return "Studio Gib Ihm!";
        case 157: return "One Click And It's Perfect!";
        case 158: return "If you drive a Tesla I want you to remove all of my plugins from your computer.";
        case 159: return "Remember to drink water, senpai!";
        case 160: return "Love <3";
        case 161: return "Your journey has just begun ;)";
        case 162: return "You will never be the same again...";
        case 163: return "Black holes are just functors that create this universe inside of this universe.";
        case 164: return "Feel the heat!";
        case 165: return "Rightclick on the randomizer to go back to the last seed.";
        case 166: return "Tbh, that would be crazy.";
        case 167: return "Your horoscope said you'll make the best beat of all time today.";
        case 168: return "Do it again! :)";
        case 169: return "Vibrato is not equal vibrato, dude.";
        case 170: return "This is going to be all over the place!";
        case 171: return "Pitch and time... it belongs together.";
        case 172: return "A rainbow is actually transcendence that never dies.";
        case 173: return "It is not random. It is destiny!";
        case 174: return "Joy can enable you to change the world.";
        case 175: return "This is a very unprofessional plugin. It sucks the 'pro' out of your music.";
        case 176: return "They tried to achieve perfection, but they didn't hear this beat yet.";
        case 177: return "Dream.";
        case 178: return "Music is a mirror of your soul and has the potential to heal.";
        case 179: return "Lmao, nxt patch is going to be garbage!";
        case 180: return "Insanity is doing the same thing over and over again.";
        case 181: return "If you're uninspired just do household-chores. Your brain will try to justify coming back.";
        case 182: return "You are defining the future standard!";
        case 183: return "Plugins are a lot like games, but you can't speedrun them.";
        case 184: return "When applying NEL's audiorate mod to mellow sounds it can make them sorta trumpet-ish.";
        case 185: return "NEL's oversampling is lightweight and reduces aliasing well, but it also alters the sound a bit.";
        case 186: return "This message was added 2022_03_15 at 18:10! just in case you wanted to know..";
        case 187: return "This is message no 187. Ratatatatatat.";
        case 188: return "Did you ever look at some font like 'This is the font I wanna use for the rest of my life'? Me neither.";
        case 189: return "Rightclick on the power button and lock its parameter to prevent it from being randomized.";
        case 190: return "idk...";
        case 191: return "This is just a placeholder for a real tooltip message.";
        case 192: return "Let's drift into a new soundscape!";
        case 193: return "This is the most generic tooltip.";
        case 194: return "Diffusion can be inharmonic, yet soothing.";
        case 195: return "I always wanted to make a plugin that has something to do with temperature..";
        case 196: return "You can't spell 'random' without 'awesome'.";
        case 197: return "Do you want a 2nd opinion on that?";
        case 198: return "This is essentially gambling, but without wasting money.";
        case 199: return "You can lock parameters in order to avoid randomizing them.";
        case 200: return "Right-click parameters in order to find additional options.";
        case 201: return "Turn it up to 11 for turbo mode! Oh, oops, wrong parameter. sryy";
        case 202: return "Bleep bloop. :> I am a computer! Hihi";
        case 203: return "Mold is basically just tiny mushrooms. You should still not eat it tho.";
        case 204: return "I wish there was a producer anime, where they'd fight for the best beats.";
        case 205: return "These tooltip messages have a deep lore.";
        case 206: return "You can't spell 'awesome' without 'random'. Well, you can, but you shouldn't.";
        case 207: return "Not in the mood today. Please use a different button! :/";
        case 208: return "I know you really want to click on this button, but I want you to try something else instead.";
        case 209: return "Sweet.";
        case 210: return "OMG I wanna tell you everything about me and my parameter-friends!!!";
        case 211: return "Take your phone number and add or subtract 1 from it! That's your textdoor neighbor.";
        case 212: return "Um.. ok?";
        case 213: return "I need more coffee.";
        case 214: return "If you feel like supporting me, you can find my Paypal link in my video descriptions. :>";
        case 215: return "Beware! This button causes quantum entanglement.";
        case 216: return "Pink is the craziest colour, because it's a mix between the lowest and highest perceivable frequencies of light.";
        case 217: return "You can't see mirrors. You can only see what mirrors show. What does a mirror look like?";
        case 218: return "If I was you, I would try to make a beat that sounds like a randomizer.";
        case 219: return "I am a computer. I am not a human. I am not a human. I am not a human. I am not a human. I am not a";
        case 220: return "In the future there will be more tooltip messages.";
        case 221: return "Refresh yourself with a cold shower!";
        case 222: return "This is the last tooltip message. I promise.";
        case 223: return "Insanity is the only way to achieve perfection.";
        case 224: return "Destructive forces cause constructive changes.";
        case 225: return "Division by Zero is neither undefined nor infinite, but simply insane.";
        case 226: return "Hold shift as you click this button to randomize sensitively.";
        case 227: return "Did you know you can hold shift to randomize sensitively?";
        case 228: return "If you hold shift while clicking on this button, it will randomize sensitively.";
        case 229: return "Mirror, mirror on the wall, who is the best producer of them all?";
        case 230: return "Shake things up and see where the music takes you!";
        case 231: return "Feeling stuck? Let chance be your guide.";
        case 232: return "Why make choices when the computer can do it for you?";
        case 233: return "Who needs a muse when you have a randomizer button?";
        case 234: return "Go ahead, live dangerously and randomize all.";
        case 235: return "Feeling uninspired? Let randomization be your DJ.";
        case 236: return "Unlock the secrets of the universe with the press of a button.";
        case 237: return "Are you ready for a mind-bending musical experience?";
        case 238: return "The randomize button holds the key to unlocking your true potential.";
        case 239: return "Unleash the power of the unknown with a press of the button.";
        case 240: return "Do you dare to uncover the hidden melodies within your music?";
        case 241: return "The randomize button is your gateway to a new dimension of sound.";
        case 243: return "I don't get high of drugs. I get high of randomization.";
        case 244: return "Randomization, like, totally changes your perspective, man.";
        case 245: return "The randomize button, like, totally expands your mind and stuff.";
        case 246: return "In the mood for some sonic wilderness?";
        case 247: return "Because man-made melodies are so last season, press randomize for a fresh natural sound.";
        case 248: return "Ready to add a little sparkle to your music?";
        case 249: return "Why play it safe when you can be a little naughty?";
        case 250: return "Creating music is like catching a dream.";
        case 251: return "Every note is a door to an alternate reality.";
        case 252: return "You never know what you will find if you dive deep enough into the sound.";
        case 253: return "Music is a journey that starts where words end.";
        case 254: return "Every chord is a story waiting to be told.";
        case 255: return "I'm sorry for the tooltip messages that were suggested by ChatGPT.";
        case 256: return "In order to make awesome beats, you have to drive a Miata!";
		case 257: return "The Miata is the best car for making beats.";
        case 258: return "You shouldn't ever make beats in a bad mood, because your car could spin out.";
		case 259: return "The best beats are made in a Miata.";
		case 260: return "MIATA POWER!!?!";
		case 261: return "The Mazda Miata has a perfect 50/50 weight distribution.";
		case 262: return "The Mazda Miatas is an affordable and lightweight sports cars with exceptional balance.";
		case 263: return "What does your music sound like in a Miata?";
        case 264: return "Click this to activate celebrity mode!";
        case 265: return "I don't need drugs to get high. I get high of plugins.";
        case 266: return "I wouldn't randomize now if I were you.. but I mean, it's your music.";
        case 267: return "This is not my final form yet.";
        default: "Are you sure?";
        }
        return "You are not supposed to read this message!";
    }
}
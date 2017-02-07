#include "core/file.h"
#include "core/oxygine.h"
#include "res/Resources.h"
#include "res/CreateResourceContext.h"

#include "SoundPlayer.h"
#include "Sound.h"
#include "SoundInstance.h"
#include "SoundSystem.h"

#include "ResSound.h"
#include "oal/StaticSoundHandleOAL.h"

#include <algorithm>

namespace oxygine
{
    void SoundPlayer::initialize()
    {
        Resources::registerResourceType(&ResSound::createResSound, "sound");
    }

    void SoundPlayer::free()
    {
        Resources::unregisterResourceType("sound");

    }

    SoundPlayer::SoundPlayer(): _resources(0), _volume(1.0f), _time(0), _lastUpdateTime(0), _paused(false)
    {
        _time = getTimeMS();
        _lastUpdateTime = _time;
    }

    SoundPlayer::~SoundPlayer()
    {

    }

    void SoundPlayer::setVolume(float v)
    {
        _volume = v;
        for (playingSounds::iterator i = _sounds.begin(); i != _sounds.end(); ++i)
        {
            SoundInstance* s = (*i).get();
            s->_updateVolume();
        }

        for (playingSounds::iterator i = _pausedSounds.begin(); i != _pausedSounds.end(); ++i)
        {
            SoundInstance* s = (*i).get();
            s->_updateVolume();
        }

    }

    void SoundPlayer::removeSoundInstance(SoundInstance* s)
    {
        playingSounds::iterator i = std::find(_sounds.begin(), _sounds.end(), s);
        //OX_ASSERT(i != _sounds.end());
        if (i != _sounds.end())
            _sounds.erase(i);
    }

    void SoundPlayer::addSoundInstance(SoundInstance* s)
    {
        playingSounds::iterator i = std::find(_sounds.begin(), _sounds.end(), s);
        //OX_ASSERT(i == _sounds.end());
        if (i == _sounds.end())
            _sounds.push_back(s);
    }


    void SoundPlayer::_onSoundDone(void* sound_instance, Channel* channel, const sound_desc& desc)
    {
        SoundInstance* t = (SoundInstance*)sound_instance;
        t->finished();
    }

    void SoundPlayer::_onSoundAboutDone(void* sound_instance, Channel* channel, const sound_desc& desc)
    {
        SoundInstance* t = (SoundInstance*)sound_instance;
        t->aboutDone();
    }

    void SoundPlayer::onSoundAboutDone(SoundInstance* soundInstance, Channel* channel, const sound_desc& desc)
    {
        /*
        soundInstance->_channel = 0;
        removeSoundInstance(soundInstance);
        */
    }

    spSoundInstance SoundPlayer::getSoundByIndex(int index)
    {
        return _sounds[index];
    }

    void SoundPlayer::setResources(Resources* res)
    {
        _resources = res;
    }





    spSoundInstance SoundPlayer::play(Resource* res, const PlayOptions& opt)
    {
        ResSound* ressound = safeCast<ResSound*>(res);
        if (!ressound || !ressound->getSound())
            return 0;


        SoundHandle* handle = SoundHandleOAL::create(ressound->getSound());
        spSoundInstance s = new SoundInstance(this, handle);

        s->setName(ressound->getPath());

        s->setPitch(opt._pitch);
        s->setLoop(opt._looped);
        if (opt._seek)
            s->seek(opt._seek);
        s->_updateVolume();

        if (opt._fadeIn)
            s->fadeIn(opt._fadeIn);
        else if (!opt._paused)
            s->play();


        return s;
    }

    spSoundInstance SoundPlayer::play(const std::string& id, const PlayOptions& opt)
    {
        if (!_resources)
            return 0;

        ResSound* res = _resources->getT<ResSound>(id);
        if (!res)
            return 0;

        return play(res, opt);
    }

    spSoundInstance SoundPlayer::continuePlay(Resource* res, Channel* ch, const PlayOptions& opt)
    {
        return 0;
        /*
        spSoundInstance s = prepareSound(res, ch, opt);
        if (!s)
            return 0;

        _sounds.push_back(s);
        ch->continuePlay(s->_desc);

        return s;
        */
    }

    void SoundPlayer::pause()
    {
        _pausedSounds.insert(_pausedSounds.end(), _sounds.begin(), _sounds.end());
        for (playingSounds::iterator i = _pausedSounds.begin(); i != _pausedSounds.end(); ++i)
        {
            SoundInstance* s = (*i).get();
            s->pause();
        }
        _paused = true;
    }

    void SoundPlayer::resume()
    {
        for (playingSounds::iterator i = _pausedSounds.begin(); i != _pausedSounds.end(); ++i)
        {
            SoundInstance* s = (*i).get();
            s->resume();
        }
        _pausedSounds.clear();
        _paused = false;
    }

    void SoundPlayer::stopByID(const string& id)
    {
        /*
        bool try_again = true;
        while (try_again)
        {
            try_again = false;

            for (playingSounds::iterator i = _sounds.begin(); i != _sounds.end(); ++i)
            {
                spSoundInstance s = *i;
                if (!s->_channel)
                    continue;

                if (s->_desc.id == id)
                {
                    s->_channel->stop();
                }
            }
        }
        */
    }

    void SoundPlayer::stop()
    {
        while (!_sounds.empty())
            _sounds.back()->stop();
        /*
        for (playingSounds::iterator i = _sounds.begin(); i != _sounds.end(); ++i)
        {
            spSoundInstance sound = *i;
            sound->stop();
        }
        */

    }

    void SoundPlayer::fadeOut(int ms)
    {
        for (playingSounds::iterator i = _sounds.begin(); i != _sounds.end(); ++i)
        {
            spSoundInstance sound = *i;
            sound->fadeOut(ms);
        }
    }

    unsigned int SoundPlayer::getTime()const
    {
        return _time;
    }

    void SoundPlayer::update()
    {
        timeMS t = getTimeMS();
        if (!_paused)
            _time += t - _lastUpdateTime;


        for (size_t i = 0; i < _sounds.size();)
        {
            bool end = false;
            {
                spSoundInstance s = _sounds[i];
                s->update();
                end = s->getState() == SoundInstance::Ended;
            }
            if (end)
                _sounds.erase(_sounds.begin() + i);
            else
                ++i;
        }

        //log::messageln("sounds %d", _sounds.size());

        _lastUpdateTime = t;
    }
}
#ifndef JELLYOS_JELLCONFIG_HPP
#define JELLYOS_JELLCONFIG_HPP

class JellConfig {

public:

    enum class DisplayMode
    {
        micLevelCheck,
        LEDChannelTest,
        Mic_NField,
        Ambient_Rainbow,
        Ambient_Deepsea,

        Count
    };

    static constexpr int NUMBER_LEDS_IN_RING = 96;

    static constexpr int NUMBER_LEDS_IN_EACH_TENTACLE = 12;

    static constexpr auto LED_ORDER_RING  = ColourOrder::RGB;

    static constexpr auto LED_ORDER_TENTACLE  = ColourOrder::RGB;

    static constexpr float BRIGHTNESS_MODIFIER = 1.0f;

    static constexpr auto DEFAULT_DISPLAY_MODE = DisplayMode::Mic_NField;
};
#endif

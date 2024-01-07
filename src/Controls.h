#include <BulbData.h>
#include <FastLED.h>

#define NUM_LEDS 12
#define NUM_LEDS 12
#define DATA_PIN 26
CRGB leds[NUM_LEDS];

void initializeLeds()
{
    FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
}

int normalizeBrightnessValue(double brightness) {
    // Ensure the brightness value is within the valid range
    int normalizedValue = static_cast<int>(min(max(brightness, 0.0), 255.0));

    // Normalize the brightness value to the range [0, 1]
    double normalizedRatio = normalizedValue / 255.0;

    // Apply gamma correction
    const double gamma = 2.0;
    double correctedValue = pow(normalizedRatio, gamma);

    // Convert the corrected value back to the original range
    return static_cast<int>(round(correctedValue * 255));
}

void setLedColor(ColorData colorData)
{
    Serial.print("Setting color to ");
    Serial.println(colorData.color.prettyString());

    fill_solid(leds, NUM_LEDS, colorData.color.toCRGB());

    FastLED.setBrightness(normalizeBrightnessValue(colorData.brightness));
    FastLED.show();
}

void adjustLights(BulbData bulbData)
{
    bool colorEnabled = !bulbData.colorNull() && bulbData.colorData.enabled;
    bool animationEnabled = !bulbData.animationNull() && bulbData.animationData.enabled;

    if (colorEnabled)
    {
        setLedColor(bulbData.colorData);
    }
    else if (animationEnabled)
    {
        Serial.println("Animations not implemented yet");
    }
    else if (!colorEnabled && !animationEnabled)
    {
        FastLED.setBrightness(0);
        FastLED.show();
    }
}
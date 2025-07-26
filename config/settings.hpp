#pragma once

enum class Language {
    English,
    Vietnamese,
    Count
};

const char *Lang_ToString(Language lang);
Language ParseLang(const char *str);

struct settings {
    Language lang = Language::English;
};

extern settings g_settings;

void register_settings_handler();

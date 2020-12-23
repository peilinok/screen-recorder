#ifndef RECORD_DESKTOP_FACTORY
#define RECORD_DESKTOP_FACTORY

#include "record_desktop.h"

int record_desktop_new(am::RECORD_DESKTOP_TYPES type, am::record_desktop **recorder);

void record_desktop_destroy(am::record_desktop **recorder);

#endif

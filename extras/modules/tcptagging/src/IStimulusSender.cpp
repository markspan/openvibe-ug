#include "CStimulusSender.h"

OV_API TCPTagging::IStimulusSender* TCPTagging::CreateStimulusSender() { return new CStimulusSender(); }

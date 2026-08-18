
  // Process a block of samples in the given form
  template <class StateType, typename Sample>
  void processSynthesis (int nSamples, Sample* dest, StateType& state) const
  {
    while (--nSamples >= 0)
    {
      *dest = state.processSynthesis (*dest, *this);
      dest++;
    }
  }

void setLayoutSynthesis (const LayoutBase& proto);

void setupSynthesis (int order,
              double sampleRate,
              double centerFrequency,
              double widthFrequency) { assert(false); }

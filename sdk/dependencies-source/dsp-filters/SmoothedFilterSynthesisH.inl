
 // Process a block of samples.
  template <typename Sample>
  void processBlockSynthesis (int nSamples,
                     Sample* const* destChannelArray)
  {
    const int numChannels = this->getNumChannels();

    // If this goes off it means setup() was never called
    assert (m_remainingSamples >= 0);

    // first handle any transition samples
    int remainingSamples = std::min (m_remainingSamples, nSamples);

    if (remainingSamples > 0)	// A PRIORI, never used in our case!
    {
      // interpolate parameters for each sample
      const double t = 1. / m_remainingSamples;
      double dp[maxParameters];
      for (int i = 0; i < DesignClass::NumParams; ++i)
        dp[i] = (this->getParams()[i] - m_transitionParams[i]) * t;

      for (int n = 0; n < remainingSamples; ++n)
      {
        for (int i = DesignClass::NumParams; --i >=0;)
          m_transitionParams[i] += dp[i];

        m_transitionFilter.setParams (m_transitionParams);
        
        for (int i = numChannels; --i >= 0;)
        {
          Sample* dest = destChannelArray[i]+n;
          *dest = this->m_state[i].processSynthesis (*dest, m_transitionFilter); //not sure to be defined !!!
        }
      }

      m_remainingSamples -= remainingSamples;

      if (m_remainingSamples == 0)
        m_transitionParams = this->getParams();
    }

    // do what's left
    if (nSamples - remainingSamples > 0)
    {
      // no transition
      for (int i = 0; i < numChannels; ++i)
        this->m_design.processSynthesis (nSamples - remainingSamples,
                          destChannelArray[i] + remainingSamples,
                          this->m_state[i]);
    }
  }

  void processSynthesis (int nSamples, double* const* arrayOfChannels)
  {
    processBlockSynthesis (nSamples, arrayOfChannels);
  }

 void doSetParamsSynthesis (const Params& parameters)
  {
    if (m_remainingSamples >= 0)
    {
      m_remainingSamples = m_transitionSamples;
    }
    else
    {
      // first time
      m_remainingSamples = 0;
      m_transitionParams = parameters;
    }

    filter_type_t::doSetParamsSynthesis (parameters);
  }

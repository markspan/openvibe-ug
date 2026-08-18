
  void setParamsSynthesis (const Params& parameters)
  {
	m_params = parameters;
    doSetParamsSynthesis (parameters);
  }

  virtual void processSynthesis (int nSamples, double* const* arrayOfChannels) = 0;

  virtual void doSetParamsSynthesis (const Params& parameters) = 0;

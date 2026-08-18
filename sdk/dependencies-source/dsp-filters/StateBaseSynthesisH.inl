
    template <typename Sample>
    inline Sample processSynthesis (const Sample in, const Cascade& c)
    {
      double out = in;
      StateType* state = m_stateArray;
      Biquad const* stage = c.m_stageArray;
//      const double vsa = ac();	// => vsa = -vsa
      int i = c.m_numStages - 1;
        out = (state++)->process1 (out, *stage++, 0); // => 0 instead of vsa
      for (; --i >= 0;)
        out = (state++)->process1 (out, *stage++, 0);
      //for (int i = c.m_numStages; --i >= 0; ++state, ++stage)
      //  out = state->process1 (out, *stage, vsa);
      return Sample (out);
    }

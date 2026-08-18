
void BandPassBase::setupSynthesis (int order,
                          double sampleRate,
                          double centerFrequency,
                          double widthFrequency)
{
  m_analogProto.design (order);

  BandPassTransform (centerFrequency / sampleRate, widthFrequency / sampleRate, m_digitalProto, m_analogProto);
  
  // inversion of the numerator and denominator digital coefficients
  Cascade::setLayoutSynthesis (m_digitalProto);
}

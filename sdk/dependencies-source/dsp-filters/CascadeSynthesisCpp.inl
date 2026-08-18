
//#include <sstream>
//#include <iostream>
//#include <iomanip>

void Cascade::setLayoutSynthesis (const LayoutBase& proto)
{
  const int numPoles = proto.getNumPoles();
  m_numStages = (numPoles + 1)/ 2;
  assert (m_numStages <= m_maxStages);

  Biquad* stage = m_stageArray;
  for (int i = 0; i < m_numStages; ++i, ++stage)
	stage->setPoleZeroPair (proto[i]);
    
  applyScale (proto.getNormalGain() / 
			  std::abs (response (proto.getNormalW() / (2 * doublePi))));

  /*
  // visu Analysis Filter
  stage-=m_numStages;
  for  (int i=0; i<m_numStages; ++i, ++stage)
  {
	std::ostringstream os;
    os << "Cascade::setLayout : " << i << "\n"
	   << "Gain = " << proto.getNormalGain() / std::abs (response (proto.getNormalW() / (2 * doublePi))) << "\n"
	   << "a0[0] = " << stage->m_a0 << "\n"
       << "a1[0] = " << stage->m_a1 << "\n"
       << "a2[0] = " << stage->m_a2 << "\n"
       << "b0[0] = " << stage->m_b0 << "\n"
       << "b1[0] = " << stage->m_b1 << "\n"
       << "b2[0] = " << stage->m_b2 << "\n";
    std::cout << os.str();
  }
  */

  //manual inversion of digital coefficients
  stage-=m_numStages;
  double a0,a1,a2,b0,b1,b2;
  for (int i=0; i<m_numStages; ++i, ++stage)
  {
	a0 = stage->m_a0;
	a1 = stage->m_a1;
	a2 = stage->m_a2;
	b0 = stage->m_b0;
	b1 = stage->m_b1;
	b2 = stage->m_b2;
	stage->m_a0 = b0;
	stage->m_a1 = b1;
	stage->m_a2 = b2;
	stage->m_b0 = a0;
	stage->m_b1 = a1;
	stage->m_b2 = a2;
  }

  applyScale (proto.getNormalGain() / std::abs (response (proto.getNormalW() / (2 * doublePi))));

  /*
  // visu Synthesis Filter
  stage-=m_numStages;
  double zeros_discr, poles_discr;
  for  (int i=0; i<m_numStages; ++i, ++stage)
  {
	std::ostringstream os;
    os << "Cascade::setLayoutSynthesis : " << i << "\n"
	   << "Gain = " << proto.getNormalGain() / std::abs (response (proto.getNormalW() / (2 * doublePi))) << "\n"
	   << "a0[0] = " << stage->m_a0 << "\n"
       << "a1[0] = " << stage->m_a1 << "\n"
       << "a2[0] = " << stage->m_a2 << "\n"
       << "b0[0] = " << stage->m_b0 << "\n"
       << "b1[0] = " << stage->m_b1 << "\n"
       << "b2[0] = " << stage->m_b2 << "\n";

	  zeros_discr = stage->m_b1*stage->m_b1 - 4*stage->m_b0*stage->m_b2;
	  poles_discr = stage->m_a1*stage->m_a1 - 4*stage->m_a0*stage->m_a2;
	  os << "zeros discriminant = " << zeros_discr << "\n";
	  if(zeros_discr>=0)
	  {
		os << "zeros = " << (-stage->m_b1 + std::sqrt(zeros_discr))/(2*stage->m_b0) << " et " << (-stage->m_b1 - std::sqrt(zeros_discr))/(2*stage->m_b0) << "\n";
	  }
	  else
	  {
		os << "zeros = " << (-stage->m_b1)/(2*stage->m_b0) << "+/-i" << std::sqrt(std::abs(zeros_discr))/(2*stage->m_b0) << "\n"
		   << "zeros abs = " << std::sqrt( (-stage->m_b1)/(2*stage->m_b0)*(-stage->m_b1)/(2*stage->m_b0) + std::sqrt(std::abs(zeros_discr))/(2*stage->m_b0)*std::sqrt(std::abs(zeros_discr))/(2*stage->m_b0) ) << "\n";
	  }
	  os << "poles discriminant = " << poles_discr << "\n";
	  if(poles_discr>=0)
	  {
		os << "poles = " << (-stage->m_a1 + std::sqrt(poles_discr))/(2*stage->m_a0) << " et " << (-stage->m_a1 - std::sqrt(poles_discr))/(2*stage->m_a0) << "\n";
	  }
	  else
	  {
		os << "poles = " << (-stage->m_a1)/(2*stage->m_a0) << "+/-i" << std::sqrt(std::abs(poles_discr))/(2*stage->m_a0) << "\n"
		   << "poles abs = " << std::sqrt( (-stage->m_a1)/(2*stage->m_a0)*(-stage->m_a1)/(2*stage->m_a0) + std::sqrt(std::abs(poles_discr))/(2*stage->m_a0)*std::sqrt(std::abs(poles_discr))/(2*stage->m_a0) ) << "\n";
	  }
    std::cout << os.str();
  }
  */

}

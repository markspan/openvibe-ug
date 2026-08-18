.. _Doc_BoxAlgorithm_ContinuousWaveletAnalysis:

Continuous Wavelet Analysis
===========================

.. container:: attribution

   :Author:
      Quentin Barthelemy
   :Company:
      Mensia Technologies

.. image:: images/Doc_BoxAlgorithm_ContinuousWaveletAnalysis.png

Performs a Time-Frequency Analysis using Continuous Wavelet Transform.

The Continuous Wavelet Transform (CWT) provides a Time-Frequency representation of an input signal, using Morlet, Paul or derivative of Gaussian wavelets.

Considering an input signal :math:`X \in \mathbb{R}^{C \times N}`, composed of :math:`C` channels and :math:`N` temporal samples, 
this plugin computes the CWT of this signal :math:`\Phi \in \mathbb{C}^{C \times F \times N}`, composed of :math:`C` channels, :math:`F` scales and :math:`N` temporal samples.
For the :math:`c^{ \text{th} }` channel, the :math:`f^{ \text{th} }` scale :math:`s_f` and the :math:`n^{ \text{th} }` sample, the Time-Frequency representation is defined as:

:math:`\Phi (c,f,n) = \sum_{n'=0}^{N-1} X(c,n') \ \psi^{*} \left( \frac{(n-n') \delta t}{s_f} \right)`,

where :math:`\psi` is the normalized wavelet, :math:`(.)^{*}` is the complex conjugate and :math:`\delta t` is the sampling period.

Using the inverse relation between wavelet scale :math:`s_f` and Fourier frequency :math:`\text{freq}_f`, output is finally defined as:

:math:`\Phi(c,f,n) = \Phi_r(c,f,n) + \mathsf{i} \times \Phi_i(c,f,n) = \left| \Phi(c,f,n) \right| \times e^{\mathsf{i} \arg(\Phi(c,f,n))}`,

with :math:`\mathsf{i}` being the imaginary unit.

Output can be visualized with a :ref:`Doc_BoxAlgorithm_InstantBitmap3DStream`.

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input signal", "Signal"

Input signal
~~~~~~~~~~~~

An input multichannel signal :math:`X \in \mathbb{R}^{C \times N}`, composed of :math:`C` channels and :math:`N` temporal samples.

Outputs
-------

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Amplitude", "Time-frequency"
   "Phase", "Time-frequency"
   "Real Part", "Time-frequency"
   "Imaginary Part", "Time-frequency"

Amplitude
~~~~~~~~~

An output spectral amplitude (absolute value) :math:`\left| \Phi \right| \in \mathbb{R}^{C \times F \times N}`.

Phase
~~~~~

An output spectral phase :math:`\arg(\Phi) \in \mathbb{R}^{C \times F \times N}`, in radians.

Real Part
~~~~~~~~~

An output real part of the spectrum :math:`\Phi_r \in \mathbb{R}^{C \times F \times N}`.

Imaginary Part
~~~~~~~~~~~~~~

An output imaginary part of the spectrum :math:`\Phi_i \in \mathbb{R}^{C \times F \times N}`.

.. _Doc_BoxAlgorithm_ContinuousWaveletAnalysis_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Wavelet type", "Continuous Wavelet Type", "Morlet wavelet"
   "Wavelet parameter", "Float", "4"
   "Number of frequencies", "Integer", "60"
   "Highest frequency", "Float", "35"
   "Frequency spacing", "Float", "12.5"

Wavelet type
~~~~~~~~~~~~

This setting defines the type of the wavelet: 


- Morlet:


:math:`\psi_0 (n) = \pi^{1/4} e^{\mathsf{i} \omega_0 n} e^{-n^2 / 2}`


- Paul:


:math:`\psi_0 (n) = \frac{2^m \mathsf{i}^m m!}{\sqrt{\pi(2m)!}} (1-\mathsf{i} n)^{-(m+1)}`


- derivative of Gaussian:


:math:`\psi_0 (n) = \frac{(-1)^{m+1}}{\sqrt{\Gamma(m+\frac{1}{2})}} \frac{d^m}{d n^m} (e^{-n^2 / 2})`


Wavelet parameter
~~~~~~~~~~~~~~~~~

This setting defines the wavelet parameter: 


- Morlet wavelet: nondimensional frequency :math:`\omega_0`, real positive parameter value. Values between 4.0 and 6.0 are typically used.



- Paul wavelet: order :math:`m`, positive integer values inferior to 20. Default value is 4.



- Derivative of Gaussian wavelet: derivative :math:`m`, positive even integer values. Value 2 gives the Marr or Mexican hat wavelet.


\n

Number of frequencies
~~~~~~~~~~~~~~~~~~~~~

This setting defines the number of frequencies :math:`F` of the CWT.

Highest frequency
~~~~~~~~~~~~~~~~~

This setting defines the highest frequency :math:`\text{freq}_F` of the CWT.

Frequency spacing
~~~~~~~~~~~~~~~~~

This setting is related to the frequency non-linear spacing of the CWT.

.. _Doc_BoxAlgorithm_ContinuousWaveletAnalysis_Miscellaneous:

Miscellaneous
-------------

Reference:

C Torrence and GP Compo, *A Practical Guide to Wavelet Analysis*, Bulletin of the American Meteorological Society, vol. 79, pp. 61–78, 1998


.. _Doc_BoxAlgorithm_TimeBasedEpoching:

Time based epoching
===================

.. container:: attribution

   :Author:
      Quentin Barthelemy
   :Company:
      Mensia Technologies

.. image:: images/Doc_BoxAlgorithm_TimeBasedEpoching.png

Interval can be used to control the overlap of epochs

The time based epoching box generates 'epochs', i.e. signal 'slices' which length is configurable, as is the time offset between two consecutive epochs. This box has one input and one output connectors, both of which are of 'signal' type. This box is essential to other signal processing boxes when the size of data blocks being forwarded to them is not significant enough.

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input signal", "Signal"

Input signal
~~~~~~~~~~~~

Input signal #1.

Outputs
-------

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Epoched signal", "Signal"

Epoched signal
~~~~~~~~~~~~~~

Epoched signal #1.

.. _Doc_BoxAlgorithm_TimeBasedEpoching_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Epoch duration (in sec)", "Float", "1"
   "Epoch intervals (in sec)", "Float", "0.5"

Epoch duration (in sec)
~~~~~~~~~~~~~~~~~~~~~~~

Length of epoched signal #1

Epoch intervals (in sec)
~~~~~~~~~~~~~~~~~~~~~~~~

Time interval between two consecutive epochs for signal #1

.. _Doc_BoxAlgorithm_TimeBasedEpoching_Examples:

Examples
--------

Practical example : apply time based epoching to compute the power spectrum of a signal

For the spectral analysis to work properly, signal data must come in chunks big enough for the analysis to be meaningful. Let's see how the time-based epoching box can help to improve the power spectrum computation of a signal. 

First, we add a Signal Oscillator box to a scenario, connect it to a Spectral Analysis box, and connect the Amplitude output connector to the 
input of a Power Spectrum Display box. Let's use default Sinus Oscillator settings (512Hz sampling frequency, data blocks size of 32 samples) and 
make sure the Amplitude setting is enabled in the Spectral Analysis box. Now we can launch the player : the power spectrum is very coarse. 
This is because the Sinus Oscillator generates small data blocks (32/512 = 1/16th of a second per block) compared to the periods of sinusoids making up the signal. The spectral analysis yields very coarse results when working on such blocks (see image below).

.. figure:: images/timebasedepoching_1.png
   :alt: Coarse power spectrum computation due to small data blocks.
   :align: center

   Coarse power spectrum computation due to small data blocks.

One way to correct this problem is to increase the data blocks size. Let's send bigger blocks by setting their size to 512 samples. When launching the player again, the power spectrum should be much finer than before, since the spectral analysis works on blocks representing 1 second of signal. However, notice how the spectrum is only refreshed at 1Hz now. This solution is not satisfactory.

.. figure:: images/timebasedepoching_2.png
   :alt: Finer power spectrum computation by sending bigger chunks.
   :align: center

   Finer power spectrum computation by sending bigger chunks.

Now we insert a Time based epoching box before the Spectral Analysis. We reset the Sinus Oscillator settings to 512 samples a second and blocks of 32 samples. Let's now setup the epoching box : we are going to generate epochs of 1 second every 1/16th of a second. Now let's launch the player again : the power spectrum is refined and updated regularly.

.. figure:: images/timebasedepoching_3.png
   :alt: Epoching 1-second chunks to refine spectrum computations
   :align: center

   Epoching 1-second chunks to refine spectrum computations

The stimulation based epoching box is similar to time based epoching, only it generates epochs when a given stimulation is received. Thus, the box has two input connectors : one for signals and another for stimulations. Settings include epoch size, epoch offset (delay when epoching should start after the target stimulation is received), and stimulation identifier.


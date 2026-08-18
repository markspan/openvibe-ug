.. _Doc_BoxAlgorithm_SimpleDSP:

Simple DSP
==========

.. container:: attribution

   :Author:
      Bruno Renier / Yann Renard
   :Company:
      Inria / IRISA

.. image:: images/Doc_BoxAlgorithm_SimpleDSP.png

This plugin is used to apply a mathematical formulae to each sample of an incoming signal and output
the resulting signal. It thus acts as a simple DSP.

The author may add up to 15 additional inputs.
In such circumstances, each input would be identified
by a letter from \e A to \e P.

Also the type of the inputs could be changed to any
streamed matrix derived type. Thus you can process
signal, spectrum or feature vector if you need.

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input - A", "Signal"

You can use from 1 to 16 inputs.

Input - A
~~~~~~~~~

Input signal

Outputs
-------

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Output", "Signal"

Output
~~~~~~

Filtered signal.

.. _Doc_BoxAlgorithm_SimpleDSP_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Equation", "String", "x"

Equation
~~~~~~~~

Formula to apply to incoming data (identified as 'X'). See :ref:`Doc_BoxAlgorithm_SimpleDSP_Miscellaneous` for more details.

.. _Doc_BoxAlgorithm_SimpleDSP_Examples:

Examples
--------

Let's consider that we want to compute the natural logarithm of the absolute value
of the input signal plus one. We just have to type the equation like that :

.. code::

   log(abs(X) + 1)

Another example : if you want to sum the cosinus of X minus Pi with its sinus plus Pi,
you can enter this equation :

.. code::

   cos(X - M_PI) + sin(X + M_PI)

.. _Doc_BoxAlgorithm_SimpleDSP_Miscellaneous:

Miscellaneous
-------------

The equation can use at most 16 variables, for 16 input signals.
The variable names are the 16 first letters of the alphabet, i.e. 'a' (or 'A') to 'p' (or 'P') matches inputs 1 to 16.
**NB** : The first input variable can be named 'x' or 'X'.

Here is a list of supported functions/operators :


- Operators:
    - ``+``
    - ``-``
    - ``*``
    - ``/``

- Unary functions
    - ``abs`` (absolute value)
    - ``acos`` (arc cosinus, requires n in the range [-1:1], result ranged in [0:M_PI]) 
    - ``asin`` (arc sinus, requires n in the range [-1:1], result ranged in [-M_PI_2:M_PI_2])
    - ``atan`` (arc tangent, requires n in the range [-1:1], result ranged in [-M_PI_2:M_PI_2])
    - ``ceil`` (upper-bound rounding)
    - ``cos`` (cosinus, n in radians, result ranged in [-1:1]) 
    - ``exp`` (exponential) 
    - ``floor`` (lower-bound rounding) 
    - ``log`` (natural logarithm, requires n>0)
    - ``log10`` (decimal logarithm, requires n>0) 
    - ``rand`` (pseudo-random, result ranged in [0:n]) 
    - ``sin`` (sinus, n in radians, result ranged in [-1:1]) 
    - ``sqrt`` (square root, requires n>=0) 
    - ``tan`` (tangent, n in radians, result ranged in [-1:1]) 

- Binary function
    - ``pow`` (power)

- Comparison operators
    - ``>``
    - ``>=``
    - ``<``
    - ``<=``
    - ``==``
    - ``=`` (equivalent to ==)
    - ``!=``
    - ``<>`` (equivalent to !=)

- Boolean operators
    - ``&`` as \e and
    - ``&&`` also as \e and
    - ``|`` as \e or
    - ``||`` also as \e or
    - ``!`` as \e not
    - ``~`` as \e xor
    - ``^`` also as \e xor

- ternary operator
    - ``? :``


There are also a few defined constants :


- ``M_PI``
- ``M_PI_2``
- ``M_PI_4``
- ``M_1_PI``
- ``M_2_PI``
- ``M_2_SQRTPI``
- ``M_SQRT2``
- ``M_SQRT1_2``
- ``M_E``
- ``M_LOG2E``
- ``M_LOG10E``
- ``M_LN2``
- ``M_LN10``


(note : their meaning is the same as the constants of the same name in math.c)

Furthermore, the equation parser is totally case-insensitive. So you can write ``COS(m_pi+x)`` or ``cos(M_PI+X)``, it doesn't matter.

Don't worry about the whitespaces and blank characters, they are automatically skipped by the equation parser.
That means, for instance, that both ``X+1`` and ``X      + 1`` work.

This plugin implements basic constant folding. That means that when the plugin analyses the equation,
if it can compute some parts of it before compilation, it will. For now, it does not support rational
equations simplification.


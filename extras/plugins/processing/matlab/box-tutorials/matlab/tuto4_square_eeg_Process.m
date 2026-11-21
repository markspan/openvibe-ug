% tuto4_square_eeg_Process.m
% -------------------------------
% UG fork: new file, not in upstream OpenViBE. See UG.md, 2.57.
%
% Called at the box's clock frequency. Squares every sample of the EEG channels
% and passes the other channels on unchanged.
%
% A signal stream carries channel names only, so a channel's type is taken from
% its name, by the rule the Signal display uses to scale "By type"
% (simple-visualization/src/SignalScaling.h, channelKind()). Change both together.
%
function box_out = tuto4_square_eeg_Process(box_in)

	for i = 1:OV_getNbPendingInputChunk(box_in, 1)

		[box_in, start_time, end_time, matrix_data] = OV_popInputBuffer(box_in, 1);

		% The output has the input's channels, names and sampling rate.
		box_in.outputs{1}.header = box_in.inputs{1}.header;

		if isempty(box_in.user_data.is_eeg)
			names = box_in.inputs{1}.header.channel_names;
			box_in.user_data.is_eeg = cellfun(@is_eeg_channel, names);
			fprintf('Square EEG: squaring %s; passing on unchanged: %s.\n', ...
				name_list(names(box_in.user_data.is_eeg)), name_list(names(~box_in.user_data.is_eeg)));
		end

		% matrix_data has one row per channel, one column per sample.
		eeg = box_in.user_data.is_eeg;
		matrix_data(eeg, :) = matrix_data(eeg, :) .^ 2;

		box_in = OV_addOutputBuffer(box_in, 1, start_time, end_time, matrix_data);
	end

	box_out = box_in;
end

% True for a channel that records EEG, as far as its name tells. Not EEG:
% trigger, status and counter channels (Trigger, SampleCounter, DIG, Saw, ...),
% EOG (HEOG, VEOG, HEO, VEO), and other body signals and amplifier inputs
% (ECG, EMG, GSR, Resp, BIP1, AUX1, ...). Every other name is an electrode.
function eeg = is_eeg_channel(name)
	n = lower(name(isstrprop(name, 'alphanum')));
	has = @(part) ~isempty(strfind(n, part));
	starts = @(part) strncmp(n, part, numel(part));

	eeg = false;
	if any(cellfun(has, {'trig', 'dig', 'status', 'counter', 'saw', 'marker', 'event', 'button'}))
		return
	end
	if has('eog') || starts('heo') || starts('veo')
		return
	end
	if any(cellfun(starts, {'ecg', 'ekg', 'emg', 'gsr', 'eda', 'resp', 'temp', 'ppg', 'pleth', 'bip', 'aux', 'acc'}))
		return
	end
	eeg = true;
end

function text = name_list(names)
	if isempty(names)
		text = 'none';
	else
		text = strjoin(names, ', ');
	end
end

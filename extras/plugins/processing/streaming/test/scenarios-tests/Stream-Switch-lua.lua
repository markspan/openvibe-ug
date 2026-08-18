
function initialize(box)
	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")

	first = OVTK_StimulationId_Label_00
	second = OVTK_StimulationId_Label_01
end

function process(box)
	local t = 0

	-- manages Timeout
	for i=1,5 do 
		box:send_stimulation(1, first, t, 0)
		t = t + 0.5
		box:send_stimulation(1, second, t, 0)
		t = t + 0.5
	end
end

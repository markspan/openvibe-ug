function initialize(box)
	dofile(box:get_config("${Path_Data}") .. "/plugins/stimulation/lua-stimulator-stim-codes.lua")
end
function uninitialize(box)
end
function process(box)
    box:send_stimulation(1, OVTK_StimulationId_ExperimentStart, 10, 0)
    box:send_stimulation(1, OVTK_StimulationId_ExperimentStop, 35, 0)
end
MyALTGX_RECONFIG_inst : MyALTGX_RECONFIG PORT MAP (
		offset_cancellation_reset	 => offset_cancellation_reset_sig,
		reconfig_clk	 => reconfig_clk_sig,
		reconfig_fromgxb	 => reconfig_fromgxb_sig,
		busy	 => busy_sig,
		reconfig_togxb	 => reconfig_togxb_sig
	);

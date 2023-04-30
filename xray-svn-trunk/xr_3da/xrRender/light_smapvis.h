#pragma	once

class smapvis : public R_feedback
{
private:
	enum
	{
		state_counting	= 0,
		state_working	= 1,
		state_usingTC	= 3,
	}							state;

	xr_vector<dxRender_Visual*>	invisible;

	u32							frame_sleep;
	u32							countOfVisualsToTest_;
	u32							indexOfVisualWeWantToTest_;
	dxRender_Visual*			testQ_V;
	u32							testQ_id;
	u32							testQ_frame;

	bool						occFlushed_;

	std::atomic<bool>			needInvalidate;

	void			invalidate();
public:
	smapvis			();
	~smapvis		();

	void			need_invalidate();
	void			smvisbegin	(DsGraphBuffer& render_buffer);			// should be called before 'marker++' and before graph-build
	void			smvisend	(DsGraphBuffer& render_buffer);
	void			smmark		(DsGraphBuffer& render_buffer);
	void			flushoccq	();			// should be called when no rendering of light is supposed

	void			resetoccq	();

	IC	bool		sleep()			{ return CurrentFrame() > frame_sleep; }

	virtual void	rfeedback_static	(dxRender_Visual* V);
};

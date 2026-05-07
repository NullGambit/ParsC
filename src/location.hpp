#pragma once

namespace pars
{
	// represents a location within a source file
	struct Location
	{
		u32 offset;
		u32 line;
		u16 file_id;
		// u16 should be enough since when would a source file have a line so big it cant fit in a u16
		u16 column;
	};
}
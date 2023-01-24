	{ //StageId_Bopeebo
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 1,
		"\\WEEK1\\WEEK1_1.MUS;1",
		
		StageId_Fresh, STAGE_LOAD_FLAG,
		false
	},
	{ //StageId_Fresh
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 2,
		"\\WEEK1\\WEEK1_2.MUS;1",
		
		StageId_DadBattle, STAGE_LOAD_FLAG,
		true
	},
	{ //StageId_DadBattle
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 3,
		"\\WEEK1\\WEEK1_3.MUS;1",
		
		StageId_DadBattle, 0,
		false
	},
	{ //StageId_Tutorial
		//Characters
		{Char_BF_New, FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_GF_New,  FIXED_DEC(0,1),  FIXED_DEC(-15,1)}, //TODO
		{NULL,           FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 4,
		"\\WEEK1\\WEEK1_4.MUS;1",
		
		StageId_Tutorial, 0,
		false
	},
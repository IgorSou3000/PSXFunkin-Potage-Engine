	{ //StageId_Bopeebo
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Stage_New,
		
		//Song info
		"STAGE", 1,
		XA_Bopeebo, 0,
		
		StageId_Fresh, STAGE_LOAD_FLAG
	},
	{ //StageId_Fresh
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Stage_New,
		
		//Song info
		"STAGE", 2,
		XA_Fresh, 2,
		
		StageId_DadBattle, STAGE_LOAD_FLAG
	},
	{ //StageId_DadBattle
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Stage_New,
		
		//Song info
		"STAGE", 3,
		XA_Dadbattle, 0,
		
		StageId_DadBattle, 0
	},
	{ //StageId_Tutorial
		//Characters
		{Char_BF_New, FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_GF_New,  FIXED_DEC(0,1),  FIXED_DEC(-15,1)}, //TODO
		{NULL,           FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Stage_New,
		
		//Song info
		"STAGE", 4,
		XA_Tutorial, 2,
		
		StageId_Tutorial, 0
	},
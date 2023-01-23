	{ //StageId_1_1 (Bopeebo)
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 1,
		XA_Bopeebo, 0,
		
		StageId_Fresh, STAGE_LOAD_FLAG,
		false
	},
	{ //StageId_1_2 (Fresh)
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 2,
		XA_Fresh, 2,
		
		StageId_DadBattle, STAGE_LOAD_FLAG,
		true
	},
	{ //StageId_1_3 (Dadbattle)
		//Characters
		{Char_BF_New,    FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_Dad_New, FIXED_DEC(-120,1),  FIXED_DEC(100,1)},
		{Char_GF_New,     FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 3,
		XA_Dadbattle, 0,
		
		StageId_DadBattle, 0,
		false
	},
	{ //StageId_1_4 (Tutorial)
		//Characters
		{Char_BF_New, FIXED_DEC(60,1),  FIXED_DEC(100,1)},
		{Char_GF_New,  FIXED_DEC(0,1),  FIXED_DEC(-15,1)}, //TODO
		{NULL,           FIXED_DEC(0,1),  FIXED_DEC(-10,1)},
		
		//Stage background
		Back_Week1_New,
		
		//Song info
		1, 4,
		XA_Tutorial, 2,
		
		StageId_Tutorial, 0,
		false
	},
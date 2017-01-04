/*
 * gtest.c
 *
 */

#include <gtest/gtest.h>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include "../cjt188.h"
#include "../cjt188def.h"

}

class CJ188_PreProcess_g: public testing::Test {
protected:
	virtual void SetUp() {
		step =0;
		rev_tail = 0;
		rev_head = 0;
	}

	virtual void TearDown() {

	}
	INT8U	step;
	INT32U	rev_delay;
	INT32U	rev_tail,rev_head;
	INT8U	RevBuf[CJ188_MAXSIZE];
	INT8U   DataBuf[35]={0x68,0x10,0x54,0x17,0x37,0x03,0x00,0x18,0x00,0x81,0x16,
			0x1F,0x90,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,
			0x4D,0x16};
};

TEST_F(CJ188_PreProcess_g,one_frame) {
	int 	ret=0,i;
	INT8U	dealbuf[CJ188_MAXSIZE];
	INT8U	RevBuf[35];

	memset(RevBuf,0,sizeof(RevBuf));
	for(i=0;i<35;i++) {
		RevBuf[i] = DataBuf[i];
		rev_head=(rev_head + 1)%CJ188_MAXSIZE;
	}
	for(i=0;i<10;i++) {
		ret = cj188_PreProcess(&step,&rev_delay,100,&rev_tail,&rev_head,RevBuf,dealbuf);
		if(ret > 0) break;
	}
	EXPECT_NE(ret,0);
}

TEST_F(CJ188_PreProcess_g,no_one_frame) {
	int 	ret=0,i,j;
	INT8U	dealbuf[CJ188_MAXSIZE];
	INT8U	RevBuf[35];

	memset(RevBuf,0,sizeof(RevBuf));
	for(i=0;i<2;i++) {
		RevBuf[i] = DataBuf[i];
		rev_head=(rev_head + 1)%CJ188_MAXSIZE;
	}
	for(j=0;j<10;j++) {
		ret = cj188_PreProcess(&step,&rev_delay,100,&rev_tail,&rev_head,RevBuf,dealbuf);
		for(i=2;i<34;i++) {
			RevBuf[i] = DataBuf[i];
			rev_head=(rev_head + 1)%CJ188_MAXSIZE;
		}

		if(ret > 0) break;
	}
	EXPECT_EQ(ret,0);
}


class filter_g: public testing::Test {
protected:
	virtual void SetUp() {
		step =0;
		rev_tail = 0;
		rev_head = 0;
	}

	virtual void TearDown() {

	}
	INT8U	step;
	INT32U	rev_delay;
	INT32U	rev_tail,rev_head;
	INT8U	RevBuf[CJ188_MAXSIZE];
	INT8U   DataBuf[35]={0x68,0x10,0x54,0x17,0x37,0x03,0x00,0x18,0x00,0x81,0x16,
			0x1F,0x90,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,
			0x4D,0x16};
};

TEST_F(filter_g,getframe_sum_ok) {
	int 	ret=0;
	int		datalen = 35;

	ret = filter(DataBuf,datalen);
	EXPECT_EQ(ret,0);
}

TEST_F(filter_g,frame_lost_0x68) {
	int 	ret=0;
	int		datalen = 35;

	DataBuf[0] = 0x69;
	ret = filter(DataBuf,datalen);
	EXPECT_EQ(ret,ERR_LOST_0x68);
}

TEST_F(filter_g,frame_lost_0x16) {
	int 	ret=0;
	int		datalen = 35;

	DataBuf[34] = 0x69;
	ret = filter(DataBuf,datalen);
	EXPECT_EQ(ret,ERR_LOST_0x16);
}

TEST_F(filter_g,frame_sum_err) {
	int 	ret=0;
	int		datalen = 35;

	DataBuf[33] = 0x69;
	ret = filter(DataBuf,datalen);
	EXPECT_EQ(ret,ERR_SUM_ERROR);
}

TEST_F(filter_g,frame_head_len_err) {
	int 	ret=0;
	int		datalen = 10;
	INT8U	TmpDataBuf[10];
	INT8U	i;

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	for(i=0;i<10;i++) {
		TmpDataBuf[i]=DataBuf[i];
	}
	ret = filter(TmpDataBuf,datalen);
	EXPECT_EQ(ret,ERR_RCVD_LOST);
}

TEST_F(filter_g,frame_len_err) {
	int 	ret=0;
	int		datalen = 34;
	INT8U	TmpDataBuf[34];
	INT8U	i;

	memset(TmpDataBuf,0,sizeof(TmpDataBuf));
	for(i=0;i<34;i++) {
		TmpDataBuf[i]=DataBuf[i];
	}
	ret = filter(TmpDataBuf,datalen);
	EXPECT_EQ(ret,ERR_RCVD_LOST);
}

class cj188_parse_g: public testing::Test {
protected:
	virtual void SetUp() {

	}

	virtual void TearDown() {

	}
	INT8U   DataBuf[35]={0x68,0x10,0x54,0x17,0x37,0x03,0x00,0x18,0x00,0x81,0x16,
			0x1F,0x90,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,
			0x4D,0x16};
};

TEST_F(cj188_parse_g,parse_9001_frame) {
	int		ret = 0;
	cj188_Frame 	cj188_para;

	ret = cj188_parse(&cj188_para,&DataBuf[0],sizeof(DataBuf));
	EXPECT_EQ(cj188_para.MeterType,0x10);
	EXPECT_EQ(cj188_para.Addr[0],0x54);
	EXPECT_EQ(cj188_para.Addr[1],0x17);
	EXPECT_EQ(cj188_para.Addr[2],0x37);
	EXPECT_EQ(cj188_para.Addr[3],0x03);
	EXPECT_EQ(cj188_para.Addr[4],0x00);
	EXPECT_EQ(cj188_para.Addr[5],0x18);
	EXPECT_EQ(cj188_para.Addr[6],0x00);
	EXPECT_EQ(cj188_para.Ctrl,0x81);
	EXPECT_EQ(cj188_para.Length,0x16);
	EXPECT_EQ(cj188_para.DI[0],0x1f);
	EXPECT_EQ(cj188_para.DI[1],0x90);
	EXPECT_EQ(ret,0x16);
}

class cj188_ComposeFrame_g: public testing::Test {
protected:
	virtual void SetUp() {
		memset(sendbuff,0,sizeof(sendbuff));
		cj188_frame.MeterType = T_WATER_HEAT;
		cj188_frame.Addr[6] = 0x01;
		cj188_frame.Addr[5] = 0x02;
		cj188_frame.Addr[4] = 0x03;
		cj188_frame.Addr[3] = 0x04;
		cj188_frame.Addr[2] = 0x05;
		cj188_frame.Addr[1] = 0x06;
		cj188_frame.Addr[0] = 0x07;
		cj188_frame.Ctrl = WRITE_DATA;

	}

	virtual void TearDown() {

	}
	cj188_Frame 	cj188_frame;
	cj188_Para		cj188_para;

	INT8U			sendbuff[256];

};

TEST_F(cj188_ComposeFrame_g,write_a010) {
	int		ret = 0;
//	int		i;
	INT8U		testbuff[32] = {0x68,0x11,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x04,0x13,
			0xa0,0x10,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
			0x0e,0x16};


	cj188_frame.DI[0] = 0xa0;
	cj188_frame.DI[1] = 0x10;
	cj188_frame.SER = 0x33;
	cj188_para.di10.Bcd_Price1[0] = 0x01;
	cj188_para.di10.Bcd_Price1[1] = 0x02;
	cj188_para.di10.Bcd_Price1[2] = 0x03;
	cj188_para.di10.Bcd_Amount1[0] = 0x04;
	cj188_para.di10.Bcd_Amount1[1] = 0x05;
	cj188_para.di10.Bcd_Amount1[2] = 0x06;
	cj188_para.di10.Bcd_Price2[0] = 0x07;
	cj188_para.di10.Bcd_Price2[1] = 0x08;
	cj188_para.di10.Bcd_Price2[2] = 0x09;
	cj188_para.di10.Bcd_Amount2[0] = 0x10;
	cj188_para.di10.Bcd_Amount2[1] = 0x11;
	cj188_para.di10.Bcd_Amount2[2] = 0x12;
	cj188_para.di10.Bcd_Price3[0] = 0x13;
	cj188_para.di10.Bcd_Price3[1] = 0x14;
	cj188_para.di10.Bcd_Price3[2] = 0x15;
	cj188_para.di10.Bcd_Date = 0x16;

	ret = cj188_ComposeFrame(cj188_frame,cj188_para,&sendbuff[0]);

	EXPECT_FALSE(memcmp(sendbuff,testbuff,sizeof(testbuff)));
	EXPECT_EQ(ret,32);
}

TEST_F(cj188_ComposeFrame_g,write_a011) {
	int		ret = 0;
	int		i;
	INT8U		testbuff[17] = {0x68,0x11,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x04,0x04,
			0xa0,0x11,0x00,0x28,0x76,0x16};

	cj188_frame.DI[0] = 0xa0;
	cj188_frame.DI[1] = 0x11;
	cj188_para.di11.Bcd_Date = 0x28;
	ret = cj188_ComposeFrame(cj188_frame,cj188_para,&sendbuff[0]);
	for(i=0;i<17;i++) {
		fprintf(stderr,"%02x_%02x ",sendbuff[i],testbuff[i]);
	}
	EXPECT_FALSE(memcmp(sendbuff,testbuff,sizeof(testbuff)));
	EXPECT_EQ(ret,17);
}

class cj188_WaterGos_CurrData_g: public testing::Test {
protected:
	virtual void SetUp() {

	}

	virtual void TearDown() {

	}
	curr_Water_Gos	currData;
};

TEST_F(cj188_WaterGos_CurrData_g,get_currData) {
	int		ret = 0;
	INT8U	testbuff[19] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x20,0x15,0x10,0x9,0x13,
			0x39,0x56,0x12,0x13};

	memset(&currData,0,sizeof(curr_Water_Gos));
	ret = cj188_WaterGos_CurrData(&currData,&testbuff[0]);
	EXPECT_EQ(currData.totalflow[0],0x01);
	EXPECT_EQ(currData.totalflow[1],0x02);
	EXPECT_EQ(currData.totalflow[2],0x03);
	EXPECT_EQ(currData.totalflow[3],0x04);
	EXPECT_EQ(currData.totalflow_unit,0x05);
	EXPECT_EQ(currData.dayflow[0],0x06);
	EXPECT_EQ(currData.dayflow[1],0x07);
	EXPECT_EQ(currData.dayflow[2],0x08);
	EXPECT_EQ(currData.dayflow[3],0x09);
	EXPECT_EQ(currData.dayflow_unit,0x0a);
	EXPECT_EQ(currData.realtime.Year,2015);
	EXPECT_EQ(currData.realtime.Month,10);
	EXPECT_EQ(currData.realtime.Day,9);
	EXPECT_EQ(currData.realtime.Hour,13);
	EXPECT_EQ(currData.realtime.Minute,39);
	EXPECT_EQ(currData.realtime.Second,56);
	EXPECT_EQ(currData.status[0],0x12);
	EXPECT_EQ(currData.status[1],0x13);
	EXPECT_EQ(ret,19);
}

class cj188_Hot_CurrData_g: public testing::Test {
protected:
	virtual void SetUp() {

	}

	virtual void TearDown() {

	}
	curr_Hot	currData;
};

TEST_F(cj188_Hot_CurrData_g,get_currData) {
	int		ret = 0;
	INT8U	testbuff[43] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
			0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x34,0x35,0x36,
			0x20,0x15,0x10,0x9,0x13,0x39,0x56,0x23,0x24};

	memset(&currData,0,sizeof(curr_Water_Gos));
	ret = cj188_Hot_CurrData(&currData,&testbuff[0]);
	EXPECT_EQ(currData.dayhot[0],0x01);
	EXPECT_EQ(currData.dayhot[1],0x02);
	EXPECT_EQ(currData.dayhot[2],0x03);
	EXPECT_EQ(currData.dayhot[3],0x04);
	EXPECT_EQ(currData.dayhot_unit,0x05);
	EXPECT_EQ(currData.currhot[0],0x06);
	EXPECT_EQ(currData.currhot[1],0x07);
	EXPECT_EQ(currData.currhot[2],0x08);
	EXPECT_EQ(currData.currhot[3],0x09);
	EXPECT_EQ(currData.currhot_unit,0x0a);
	EXPECT_EQ(currData.hotpower[0],0x0b);
	EXPECT_EQ(currData.hotpower[1],0x0c);
	EXPECT_EQ(currData.hotpower[2],0x0d);
	EXPECT_EQ(currData.hotpower[3],0x0e);
	EXPECT_EQ(currData.hotpower_unit,0x0f);
	EXPECT_EQ(currData.flow[0],0x10);
	EXPECT_EQ(currData.flow[1],0x11);
	EXPECT_EQ(currData.flow[2],0x12);
	EXPECT_EQ(currData.flow[3],0x13);
	EXPECT_EQ(currData.flow_unit,0x14);
	EXPECT_EQ(currData.totalflow[0],0x15);
	EXPECT_EQ(currData.totalflow[1],0x16);
	EXPECT_EQ(currData.totalflow[2],0x17);
	EXPECT_EQ(currData.totalflow[3],0x18);
	EXPECT_EQ(currData.totalflow_unit,0x19);
	EXPECT_EQ(currData.servewatertemp[0],0x1a);
	EXPECT_EQ(currData.servewatertemp[1],0x1b);
	EXPECT_EQ(currData.servewatertemp[2],0x1c);
	EXPECT_EQ(currData.backwatertemp[0],0x1d);
	EXPECT_EQ(currData.backwatertemp[1],0x1e);
	EXPECT_EQ(currData.backwatertemp[2],0x1f);
	EXPECT_EQ(currData.worktime[0],0x34);
	EXPECT_EQ(currData.worktime[1],0x35);
	EXPECT_EQ(currData.worktime[2],0x36);
	EXPECT_EQ(currData.realtime.Year,2015);
	EXPECT_EQ(currData.realtime.Month,10);
	EXPECT_EQ(currData.realtime.Day,9);
	EXPECT_EQ(currData.realtime.Hour,13);
	EXPECT_EQ(currData.realtime.Minute,39);
	EXPECT_EQ(currData.realtime.Second,56);
	EXPECT_EQ(currData.status[0],0x23);
	EXPECT_EQ(currData.status[1],0x24);
	EXPECT_EQ(ret,43);
}

int main(int argc, char * argv[]) {
//	testing::GTEST_FLAG(filter) = "*";	//--gtest_filter=list*
//	testing::GTEST_FLAG(output) = "xml:/home/lhl/runxml";
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}


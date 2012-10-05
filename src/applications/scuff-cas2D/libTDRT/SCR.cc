/*--------------------------------------------------------------*/
/*- 'square cubature rules:' rules for 2D numerical cubature   -*/
/*- over the square with corners at (0,0) (1,0) (0,1) (1,1)    -*/
/*--------------------------------------------------------------*/

/* SCR=SCR3; NumPts=5;*/
double SCR3[]={
  +5.0000000000000000e-01, +5.0000000000000000e-01, -1.1111111111111110e-01,
  +8.8729833462074170e-01, +5.0000000000000000e-01, +2.7777777777777779e-01,
  +5.0000000000000000e-01, +8.8729833462074170e-01, +2.7777777777777779e-01,
  +1.1270166537925830e-01, +5.0000000000000000e-01, +2.7777777777777779e-01,
  +5.0000000000000000e-01, +1.1270166537925830e-01, +2.7777777777777779e-01 
};

/* SCR=SCR5; NumPts=8;*/
double SCR5[]={
  +8.4156502553198664e-01, +5.0000000000000000e-01, +2.0408163265306123e-01,
  +5.0000000000000000e-01, +8.4156502553198664e-01, +2.0408163265306123e-01,
  +1.5843497446801341e-01, +5.0000000000000000e-01, +2.0408163265306123e-01,
  +5.0000000000000000e-01, +1.5843497446801341e-01, +2.0408163265306123e-01,
  +9.4095855184409838e-01, +9.4095855184409838e-01, +4.5918367346938778e-02,
  +9.4095855184409838e-01, +5.9041448155901566e-02, +4.5918367346938778e-02,
  +5.9041448155901566e-02, +9.4095855184409838e-01, +4.5918367346938778e-02,
  +5.9041448155901566e-02, +5.9041448155901566e-02, +4.5918367346938778e-02 
};

/* SCR=SCR7; NumPts=12;*/
double SCR7[]={
  +9.6291004988627571e-01, +5.0000000000000000e-01, +6.0493827160493827e-02,
  +5.0000000000000000e-01, +9.6291004988627571e-01, +6.0493827160493827e-02,
  +3.7089950113724290e-02, +5.0000000000000000e-01, +6.0493827160493827e-02,
  +5.0000000000000000e-01, +3.7089950113724290e-02, +6.0493827160493827e-02,
  +6.9027721660415786e-01, +6.9027721660415786e-01, +1.3014822916684862e-01,
  +6.9027721660415786e-01, +3.0972278339584214e-01, +1.3014822916684862e-01,
  +3.0972278339584214e-01, +6.9027721660415786e-01, +1.3014822916684862e-01,
  +3.0972278339584214e-01, +3.0972278339584214e-01, +1.3014822916684862e-01,
  +9.0298989145929931e-01, +9.0298989145929931e-01, +5.9357943672657558e-02,
  +9.0298989145929931e-01, +9.7010108540700635e-02, +5.9357943672657558e-02,
  +9.7010108540700635e-02, +9.0298989145929931e-01, +5.9357943672657558e-02,
  +9.7010108540700635e-02, +9.7010108540700635e-02, +5.9357943672657558e-02 
};

/* SCR=SCR9; NumPts=17;*/
double SCR9[]={
  +5.0000000000000000e-01, +5.0000000000000000e-01, +1.3168724279835392e-01,
  +9.8442498318098881e-01, +8.1534005986583447e-01, +2.2219844542549678e-02,
  +9.8442498318098881e-01, +1.8465994013416559e-01, +2.2219844542549678e-02,
  +1.5575016819011134e-02, +8.1534005986583447e-01, +2.2219844542549678e-02,
  +1.5575016819011134e-02, +1.8465994013416559e-01, +2.2219844542549678e-02,
  +8.7513854998945029e-01, +9.6398082297978482e-01, +2.8024900532399120e-02,
  +8.7513854998945029e-01, +3.6019177020215176e-02, +2.8024900532399120e-02,
  +1.2486145001054971e-01, +9.6398082297978482e-01, +2.8024900532399120e-02,
  +1.2486145001054971e-01, +3.6019177020215176e-02, +2.8024900532399120e-02,
  +7.6186791010721466e-01, +7.2666991056782360e-01, +9.9570609815517519e-02,
  +7.6186791010721466e-01, +2.7333008943217640e-01, +9.9570609815517519e-02,
  +2.3813208989278534e-01, +7.2666991056782360e-01, +9.9570609815517519e-02,
  +2.3813208989278534e-01, +2.7333008943217640e-01, +9.9570609815517519e-02,
  +5.3810416409630857e-01, +9.2630786466683113e-01, +6.7262834409945196e-02,
  +5.3810416409630857e-01, +7.3692135333168873e-02, +6.7262834409945196e-02,
  +4.6189583590369143e-01, +9.2630786466683113e-01, +6.7262834409945196e-02,
  +4.6189583590369143e-01, +7.3692135333168873e-02, +6.7262834409945196e-02 
};

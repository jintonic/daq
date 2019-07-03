void extractOffsetData(int run=0)
{
   ifstream input(Form("run_%06d.000001",run), ios::binary); 
   
   ofstream output("offset.dat",ofstream::app);

   unsigned int hdr[1000], raw;
   input.read(reinterpret_cast<char *> (&hdr),(4+10+4)*4);
   output<<(hdr[13]&0xffff)<<" "; // set offset

   int wfsize=(hdr[14]-16)/4; // individual waveform size in bytes
   input.read(reinterpret_cast<char *> (&raw),4);
   output<<(raw&0x3FF)<<" ";

   input.seekg((4+10+4)*4+wfsize); // second waveform
   input.read(reinterpret_cast<char *> (&raw),4);
   output<<(raw&0x3FF)<<" ";

   input.seekg((4+10+4)*4+wfsize*2); // third waveform
   input.read(reinterpret_cast<char *> (&raw),4);
   output<<(raw&0x3FF)<<" ";

   input.seekg((4+10+4)*4+wfsize*3); // last waveform
   input.read(reinterpret_cast<char *> (&raw),4);
   output<<(raw&0x3FF)<<endl;

   input.close();
   output.close();
}

void offset(int lastRun=1)
{
   for (int run=0; run<lastRun; run++) extractOffsetData(run);
   
   TTree *t = new TTree("t","");
   t->ReadFile("offset.dat","x:c0:c1:c2:c3");
   
   TGraph *g[4] = {0};
   for (int i=0; i<4; i++) {
      t->Draw(Form("c%d:x",i),Form("x>0 && x<65535 && c%d<1023",i));
      g[i] = new TGraph(t->GetSelectedRows(), t->GetV1(), t->GetV2());
      g[i]->SetTitle("");
      g[i]->Draw("ap");
      g[i]->Fit("pol1");
      gPad->Print(Form("ch%d.png",i));
   }
}

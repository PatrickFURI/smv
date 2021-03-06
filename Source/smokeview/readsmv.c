#include "options.h"
#include "glew.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include GLUT_H
#include <pthread.h>

#include "smv_endian.h"
#include "update.h"
#include "smokeviewvars.h"
#include "IOvolsmoke.h"

#ifdef pp_READBUFFER
#define BREAK \
      if(readfile_option==READBUFFER){\
        break;\
      }\
      else{\
        if((stream->stream==stream->stream1&&stream->stream2==NULL)||stream->stream==stream->stream2)break;\
        stream->stream=stream->stream2;\
        continue;\
      }
#else
#define BREAK \
      if((stream==stream1&&stream2==NULL)||stream==stream2)break;\
      stream=stream2;\
      continue
#endif
#define BREAK2 \
      if((stream==stream1&&stream2==NULL)||stream==stream2)break;\
      stream=stream2;\
      continue

#define COLOR_INVISIBLE -2

#define BLOCK_OUTLINE 2

#define DEVICE_DEVICE 0
#define DEVICE_HEAT 2
#define DEVICE_SPRK 3
#define DEVICE_SMOKE 4

#define ZVENT_1ROOM 1
#define ZVENT_2ROOM 2

/* ------------------ ReadHRR ------------------------ */
#define LENBUFFER 1024
void ReadHRR(int flag, int *errorcode){
  FILE *HRRFILE;
  int ntimeshrr, nfirst;
  char buffer[LENBUFFER];
  float *hrrtime, *hrrval;
  int display = 0;
  int ntimes_saved;

  *errorcode = 0;
  if(hrrinfo!=NULL){
    display = hrrinfo->display;
    FREEMEMORY(hrrinfo->times_csv);
    FREEMEMORY(hrrinfo->times);
    FREEMEMORY(hrrinfo->hrrval_csv);
    FREEMEMORY(hrrinfo->hrrval);
    FREEMEMORY(hrrinfo->timeslist);
  }
  FREEMEMORY(hrrinfo);
  if(flag==UNLOAD)return;

  NewMemory((void **)&hrrinfo, sizeof(hrrdata));
  hrrinfo->file = hrr_csv_filename;
  hrrinfo->times_csv = NULL;
  hrrinfo->times = NULL;
  hrrinfo->timeslist = NULL;
  hrrinfo->hrrval_csv = NULL;
  hrrinfo->hrrval = NULL;
  hrrinfo->ntimes_csv = 0;
  hrrinfo->loaded = 1;
  hrrinfo->display = display;
  hrrinfo->itime = 0;

  HRRFILE = fopen(hrrinfo->file, "r");
  if(HRRFILE==NULL){
    ReadHRR(UNLOAD, errorcode);
    return;
  }


  // size data

  ntimeshrr = 0;
  nfirst = -1;
  while(!feof(HRRFILE)){
    if(fgets(buffer, LENBUFFER, HRRFILE)==NULL)break;
    if(nfirst==-1&&strstr(buffer, ".")!=NULL)nfirst = ntimeshrr;
    ntimeshrr++;
  }
  ntimes_saved = ntimeshrr;
  ntimeshrr -= nfirst;

  rewind(HRRFILE);
  NewMemory((void **)&hrrinfo->times_csv, ntimeshrr*sizeof(float));
  NewMemory((void **)&hrrinfo->hrrval_csv, ntimeshrr*sizeof(float));

  // read data

  hrrtime = hrrinfo->times_csv;
  hrrval = hrrinfo->hrrval_csv;
  ntimeshrr = 0;

  // read no more than the number of lines found during first pass

  while(ntimeshrr<ntimes_saved&&!feof(HRRFILE)){
    if(fgets(buffer, LENBUFFER, HRRFILE)==NULL)break;
    if(ntimeshrr<nfirst){
      ntimeshrr++;
      continue;
    }
    StripCommas(buffer);
    sscanf(buffer, "%f %f", hrrtime, hrrval);
    hrrtime++;
    hrrval++;
    ntimeshrr++;
  }
  hrrinfo->ntimes_csv = ntimeshrr-nfirst;
  fclose(HRRFILE);
}

/* ------------------ InitProp ------------------------ */

void InitProp(propdata *propi, int nsmokeview_ids, char *label){
  int nlabel;

  nlabel = strlen(label);
  if(nlabel == 0){
    NewMemory((void **)&propi->label, 5);
    strcpy(propi->label, "null");
  }
  else{
    NewMemory((void **)&propi->label, nlabel + 1);
    strcpy(propi->label, label);
  }

  NewMemory((void **)&propi->smokeview_ids, nsmokeview_ids*sizeof(char *));
  NewMemory((void **)&propi->smv_objects, nsmokeview_ids*sizeof(sv_object *));

  propi->nsmokeview_ids = nsmokeview_ids;
  propi->blockvis = 1;
  propi->inblockage = 0;
  propi->ntextures = 0;
  propi->nvars_dep = 0;
  propi->nvars_evac = 0;
  propi->nvars_indep = 0;
  propi->vars_indep = NULL;
  propi->svals = NULL;
  propi->texturefiles = NULL;
  propi->rotate_axis = NULL;
  propi->rotate_angle = 0.0;
  propi->fvals = NULL;
  propi->vars_indep_index = NULL;
  propi->tag_number = 0;
  propi->draw_evac = 0;
}

/* ------------------ InitDefaultProp ------------------------ */

void InitDefaultProp(void){
/*
PROP
 Human_props
  4
 human_fixed
 human_altered_with_data
 ellipsoid
 disk
  1
 D=0.2
 */
  propdata *propi;
  char proplabel[255];
  int lenbuf;
  int ntextures_local;
  int nsmokeview_ids;
  char *smokeview_id;
  char buffer[255];
  int i;

  propi = propinfo + npropinfo;

  strcpy(proplabel,"Human_props(default)");           // from input

  nsmokeview_ids=4;                                   // from input

  InitProp(propi,nsmokeview_ids,proplabel);
  for(i=0;i<nsmokeview_ids;i++){
    if(i==0)strcpy(buffer,"human_fixed");             // from input
    if(i==1)strcpy(buffer,"human_altered_with_data"); // from input
    if(i==2)strcpy(buffer,"ellipsoid");               // from input
    if(i==3)strcpy(buffer,"disk");                    // from input
    lenbuf=strlen(buffer);
    NewMemory((void **)&smokeview_id,lenbuf+1);
    strcpy(smokeview_id,buffer);
    propi->smokeview_ids[i]=smokeview_id;
    propi->smv_objects[i]=GetSmvObjectType(propi->smokeview_ids[i],missing_device);
  }
  propi->smv_object=propi->smv_objects[0];
  propi->smokeview_id=propi->smokeview_ids[0];

  propi->nvars_indep=1;                               // from input
  propi->vars_indep=NULL;
  propi->svals=NULL;
  propi->texturefiles=NULL;
  ntextures_local=0;
  if(propi->nvars_indep>0){
    NewMemory((void **)&propi->vars_indep,propi->nvars_indep*sizeof(char *));
    NewMemory((void **)&propi->svals,propi->nvars_indep*sizeof(char *));
    NewMemory((void **)&propi->fvals,propi->nvars_indep*sizeof(float));
    NewMemory((void **)&propi->vars_indep_index,propi->nvars_indep*sizeof(int));
    NewMemory((void **)&propi->texturefiles,propi->nvars_indep*sizeof(char *));

    for(i=0;i<propi->nvars_indep;i++){
      char *equal;

      propi->svals[i]=NULL;
      propi->vars_indep[i]=NULL;
      propi->fvals[i]=0.0;
      if(i==0)strcpy(buffer,"D=0.2");                 // from input
      equal=strchr(buffer,'=');
      if(equal!=NULL){
        char *buf1, *buf2, *keyword, *val;
        int lenkey, lenval;
        char *texturefile;

        buf1=buffer;
        buf2=equal+1;
        *equal=0;

        TrimBack(buf1);
        keyword=TrimFront(buf1);
        lenkey=strlen(keyword);

        TrimBack(buf2);
        val=TrimFront(buf2);
        lenval=strlen(val);

        if(lenkey==0||lenval==0)continue;

        if(val[0]=='"'){
          val[0]=' ';
          if(val[lenval-1]=='"')val[lenval-1]=' ';
          TrimBack(val);
          val=TrimFront(val);
          NewMemory((void **)&propi->svals[i],lenval+1);
          strcpy(propi->svals[i],val);
          texturefile=strstr(val,"t%");
          if(texturefile!=NULL){
            texturefile+=2;
            texturefile=TrimFront(texturefile);
            propi->texturefiles[ntextures_local]=propi->svals[i];
            strcpy(propi->svals[i],texturefile);

            ntextures_local++;
          }
        }

        NewMemory((void **)&propi->vars_indep[i],lenkey+1);
        strcpy(propi->vars_indep[i],keyword);
        sscanf(val,"%f",propi->fvals+i);
      }
    }
    GetIndepVarIndices(propi->smv_object,propi->vars_indep,propi->nvars_indep,propi->vars_indep_index);
    GetEvacIndices(propi->smv_object,propi->fvars_evac_index,&propi->nvars_evac);
  }
  propi->ntextures=ntextures_local;
}

/* ------------------ UpdateINIList ------------------------ */

void UpdateINIList(void){
  char filter[256];
  int i;

  strcpy(filter,fdsprefix);
  strcat(filter,"*.ini");
  FreeFileList(ini_filelist,&nini_filelist);
  nini_filelist=GetFileListSize(".",filter);
  if(nini_filelist>0){
    MakeFileList(".",filter,nini_filelist,NO,&ini_filelist);

    for(i=0;i<nini_filelist;i++){
      filelistdata *filei;

      filei = ini_filelist + i;
      if(filei->type!=0)continue;
      InsertIniFile(filei->file);
    }
  }
}

/* ------------------ FreeLabels ------------------------ */

void FreeLabels(flowlabels *flowlabel){
  FREEMEMORY(flowlabel->longlabel);
  FREEMEMORY(flowlabel->shortlabel);
  FREEMEMORY(flowlabel->unit);
}

/* ------------------ InitMesh ------------------------ */

void InitMesh(meshdata *meshi){
  meshi->opacity_adjustments = NULL;
  meshi->light_fraction = NULL;
  meshi->uc_light_fraction = NULL;
  meshi->is_extface[0] = 1;
  meshi->is_extface[1] = 1;
  meshi->is_extface[2] = 1;
  meshi->is_extface[3] = 1;
  meshi->is_extface[4] = 1;
  meshi->is_extface[5] = 1;
  meshi->ncutcells = 0;
  meshi->cutcells = NULL;
  meshi->slice_min[0] = 1.0;
  meshi->slice_min[1] = 1.0;
  meshi->slice_min[2] = 1.0;
  meshi->slice_max[0] = 0.0;
  meshi->slice_max[1] = 0.0;
  meshi->slice_max[2] = 0.0;
  meshi->s_offset[0] = -1;
  meshi->s_offset[1] = -1;
  meshi->s_offset[2] = -1;
  meshi->super = NULL;
  meshi->nabors[0] = NULL;
  meshi->nabors[1] = NULL;
  meshi->nabors[2] = NULL;
  meshi->nabors[3] = NULL;
  meshi->nabors[4] = NULL;
  meshi->nabors[5] = NULL;
  meshi->update_smoke3dcolors = 0;
  meshi->iplotx_all = NULL;
  meshi->iploty_all = NULL;
  meshi->iplotz_all = NULL;
#ifdef pp_GPU
  meshi->smoke_texture_buffer = NULL;
  meshi->smoke_texture_id = -1;

  meshi->fire_texture_buffer = NULL;
  meshi->fire_texture_id = -1;

  meshi->light_texture_buffer = NULL;
  meshi->light_texture_id = -1;

  meshi->slice3d_texture_buffer = NULL;
  meshi->slice3d_texture_id = -1;
  meshi->slice3d_c_buffer = NULL;
#endif
  meshi->mesh_offset_ptr = NULL;
#ifdef pp_CULL
  meshi->cullinfo = NULL;
  meshi->culldefined = 0;
  meshi->cullQueryId = NULL;
  meshi->cull_smoke3d = NULL;
  meshi->smokedir_old = -100;
#endif
  meshi->cullgeominfo = NULL;
  meshi->is_bottom = 1;
  meshi->blockvis = 1;
  meshi->zcell = NULL;
  meshi->terrain = NULL;
  meshi->meshrgb[0] = 0.0;
  meshi->meshrgb[1] = 0.0;
  meshi->meshrgb[2] = 0.0;
  meshi->meshrgb_ptr = NULL;
  meshi->cellsize = 0.0;
  meshi->smokedir = 1;
  meshi->merge_alpha = NULL;
  meshi->merge_color = NULL;
  meshi->dx = 1.0;
  meshi->dy = 1.0;
  meshi->dz = 1.0;
  meshi->dxy = 1.0;
  meshi->dxz = 1.0;
  meshi->dyz = 1.0;
  meshi->label = NULL;
  meshi->maxtimes_boundary = 0;
  meshi->slicedir = YDIR;
  meshi->visInteriorBoundaries = 0;
  meshi->plot3dfilenum = -1;
  meshi->patchfilenum = -1;
  meshi->obst_bysize = NULL;
  meshi->iqdata = NULL;
  meshi->qdata = NULL;
  meshi->yzcolorbase = NULL;
  meshi->xzcolorbase = NULL;
  meshi->xycolorbase = NULL;
  meshi->yzcolorfbase = NULL;
  meshi->xzcolorfbase = NULL;
  meshi->xycolorfbase = NULL;
  meshi->yzcolortbase = NULL;
  meshi->xzcolortbase = NULL;
  meshi->xycolortbase = NULL;
  meshi->dx_xy = NULL;
  meshi->dy_xy = NULL;
  meshi->dz_xy = NULL;
  meshi->dx_xz = NULL;
  meshi->dy_xz = NULL;
  meshi->dz_xz = NULL;
  meshi->dx_yz = NULL;
  meshi->dy_yz = NULL;
  meshi->dz_yz = NULL;
  meshi->c_iblank_xy = NULL;
  meshi->c_iblank_xz = NULL;
  meshi->c_iblank_yz = NULL;
  meshi->iblank_smoke3d = NULL;
  meshi->iblank_smoke3d_defined = 0;
  meshi->animatedsurfaces = NULL;
  meshi->blockagesurface = NULL;
  meshi->blockagesurfaces = NULL;
  meshi->showlevels = NULL;
  meshi->isolevels = NULL;
  meshi->nisolevels = 0;
  meshi->iso_times = NULL;
  meshi->iso_timeslist = NULL;
  meshi->isofilenum = -1;
  meshi->nvents = 0;
  meshi->ndummyvents = 0;
  meshi->ncvents = 0;
  meshi->npatches = 0;
  meshi->boundarytype = NULL;
  meshi->offset[XXX] = 0.0;
  meshi->offset[YYY] = 0.0;
  meshi->offset[ZZZ] = 0.0;
  meshi->boundarytype = NULL;
  meshi->patchdir = NULL;
  meshi->patch_surfindex = NULL;
  meshi->pi1 = NULL;
  meshi->pi2 = NULL;
  meshi->pj1 = NULL;
  meshi->pj2 = NULL;
  meshi->pk1 = NULL;
  meshi->pk2 = NULL;
  meshi->meshonpatch = NULL;
  meshi->blockonpatch = NULL;
  meshi->ptype = NULL;
  meshi->boundary_row = NULL, meshi->boundary_col = NULL, meshi->blockstart = NULL;
  meshi->zipoffset = NULL, meshi->zipsize = NULL;
  meshi->vis_boundaries = NULL;
  meshi->xyzpatch = NULL;
  meshi->xyzpatch_threshold = NULL;
  meshi->patchventcolors = NULL;
  meshi->cpatchval = NULL;
  meshi->cpatchval_iframe = NULL;
  meshi->cpatchval_iframe_zlib = NULL;
  meshi->cpatchval_zlib = NULL;
  meshi->patch_times = NULL;
  meshi->patchval = NULL;
  meshi->patchval_iframe = NULL;
  meshi->thresholdtime = NULL;
  meshi->patchblank = NULL;
  meshi->patch_contours = NULL;
  meshi->patch_timeslist = NULL;
  meshi->ntc = 0;
  meshi->nspr = 0;
  meshi->xsprplot = NULL;
  meshi->ysprplot = NULL;
  meshi->zsprplot = NULL;
  meshi->xspr = NULL;
  meshi->yspr = NULL;
  meshi->zspr = NULL;
  meshi->tspr = NULL;
  meshi->nheat = 0;
  meshi->xheatplot = NULL;
  meshi->yheatplot = NULL;
  meshi->zheatplot = NULL;
  meshi->xheat = NULL;
  meshi->yheat = NULL;
  meshi->zheat = NULL;
  meshi->theat = NULL;
  meshi->blockageinfoptrs = NULL;

  meshi->surface_tempmax = SURFACE_TEMPMAX;
  meshi->surface_tempmin = SURFACE_TEMPMIN;

  meshi->faceinfo = NULL;
  meshi->face_normals_single = NULL;
  meshi->face_normals_double = NULL;
  meshi->face_transparent_double = NULL;
  meshi->face_textures = NULL;
  meshi->face_outlines = NULL;

  meshi->xplt = NULL;
  meshi->yplt = NULL;
  meshi->zplt = NULL;
  meshi->xvolplt = NULL;
  meshi->yvolplt = NULL;
  meshi->zvolplt = NULL;
  meshi->xplt_cen = NULL;
  meshi->yplt_cen = NULL;
  meshi->zplt_cen = NULL;
  meshi->xplt_orig = NULL;
  meshi->yplt_orig = NULL;
  meshi->zplt_orig = NULL;

  meshi->f_iblank_cell = NULL;
  meshi->c_iblank_cell = NULL;
  meshi->c_iblank_x = NULL;
  meshi->c_iblank_y = NULL;
  meshi->c_iblank_z = NULL;
  meshi->c_iblank_node = NULL;
  meshi->c_iblank_embed = NULL;
  meshi->block_zdist = NULL;

  meshi->f_iblank_cell0 = NULL;
  meshi->c_iblank_cell0 = NULL;
  meshi->c_iblank_x0 = NULL;
  meshi->c_iblank_y0 = NULL;
  meshi->c_iblank_z0 = NULL;
  meshi->c_iblank_node0 = NULL;
  meshi->c_iblank_embed0 = NULL;
  meshi->block_zdist0 = NULL;

  meshi->xyz_bar[XXX] = 1.0;
  meshi->xyz_bar0[XXX] = 0.0;
  meshi->xyz_bar[YYY] = 1.0;
  meshi->xyz_bar0[YYY] = 0.0;
  meshi->xyz_bar[ZZZ] = 1.0;
  meshi->xyz_bar0[ZZZ] = 0.0;

  meshi->xyzmaxdiff = 1.0;

  meshi->xcen = 0.5;
  meshi->ycen = 0.5;
  meshi->zcen = 0.5;

  meshi->plotx = 1;
  meshi->ploty = 1;
  meshi->plotz = 1;

  meshi->boxoffset = 0.0;
  meshi->ventinfo = NULL;
  meshi->cventinfo = NULL;
  meshi->select_min = 0;
  meshi->select_max = 0;
}

/* ------------------ GetCloseVent ------------------------ */

ventdata *GetCloseVent(meshdata *ventmesh, int ivent){
  ventdata *close_vent, *vdummy_start;
  int i;

  close_vent = ventmesh->ventinfo+ivent;
  if(close_vent->dir2==XDIR&&close_vent->imin>0&&close_vent->imax<ventmesh->ibar)return close_vent;
  if(close_vent->dir2==YDIR&&close_vent->jmin>0&&close_vent->jmax<ventmesh->jbar)return close_vent;
  if(close_vent->dir2==ZDIR&&close_vent->kmin>0&&close_vent->kmax<ventmesh->kbar)return close_vent;
  vdummy_start = ventmesh->ventinfo+ventmesh->nvents-ventmesh->ndummyvents;
  for(i = 0;i<ventmesh->ndummyvents;i++){
    ventdata *vi;

    vi = vdummy_start+i;
    if(close_vent->imin==vi->imin&&close_vent->imax==vi->imax&&
       close_vent->jmin==vi->jmin&&close_vent->jmax==vi->jmax&&
       close_vent->kmin==vi->kmin&&close_vent->kmax==vi->kmax
      ){
      return vi;
    }
  }
  return close_vent;
}

/* ------------------ ReadSMVDynamic ------------------------ */

void ReadSMVDynamic(char *file){
  int ioffset;
  float time_local;
  int i;
  int nn_plot3d=0,iplot3d=0;
  int do_pass2=0, do_pass3=0, minmaxpl3d=0;
  int nplot3dinfo_old;
#ifdef pp_READBUFFER
  bufferstreamdata streaminfo, *stream=&streaminfo;
#else
  FILE *stream=NULL;
#endif

#ifdef pp_READBUFFER
  if(readfile_option==READBUFFER){
    stream->fileinfo = File2Buffer(file);
  }
#endif

  nplot3dinfo_old=nplot3dinfo;

  updatefacelists=1;
  updatemenu=1;
  if(nplot3dinfo>0){
    int n;

    for(i=0;i<nplot3dinfo;i++){
      plot3ddata *plot3di;

      plot3di = plot3dinfo + i;
      for(n=0;n<6;n++){
        FreeLabels(&plot3di->label[n]);
      }
      FREEMEMORY(plot3di->reg_file);
      FREEMEMORY(plot3di->comp_file);
    }
//    FREEMEMORY(plot3dinfo);
  }
  nplot3dinfo=0;

#ifdef pp_READBUFFER
  if(readfile_option==READFILE){
    stream->stream1 = fopen(file, "r");
    stream->stream2 = NULL;
    stream->stream = stream->stream1;
    if(stream->stream==NULL)return;
  }
#else
  stream=fopen(file,"r");
  if(stream==NULL)return;
#endif
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;
    int j;

    meshi=meshinfo+i;
    for(j=0;j<meshi->nbptrs;j++){
      blockagedata *bc_local;

      bc_local=meshi->blockageinfoptrs[j];
      bc_local->nshowtime=0;
      FREEMEMORY(bc_local->showtime);
      FREEMEMORY(bc_local->showhide);
    }
    for(j=0;j<meshi->nvents;j++){
      ventdata *vi;

      vi = meshi->ventinfo + j;
      vi->nshowtime=0;
      FREEMEMORY(vi->showhide);
      FREEMEMORY(vi->showtime);
    }
    for(j = 0; j<meshi->ncvents; j++){
      cventdata *cvi;

      cvi = meshi->cventinfo + j;
      cvi->nshowtime = 0;
      FREEMEMORY(cvi->showhide);
      FREEMEMORY(cvi->showtime);
    }
  }
  for(i=ndeviceinfo_exp;i<ndeviceinfo;i++){
    devicedata *devicei;

    devicei = deviceinfo + i;
    devicei->nstate_changes=0;
    FREEMEMORY(devicei->act_times);
    FREEMEMORY(devicei->state_values);
  }

  ioffset=0;

  // ------------------------------- pass 1 dynamic - start ------------------------------------

  for(;;){
    char buffer[255],buffer2[255];

    if(FGETS(buffer,255,stream)==NULL)break;
    if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OFFSET ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OFFSET") == 1){
      ioffset++;
      continue;
    }
/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ PL3D ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"PL3D") == 1){
      do_pass2=1;
      if(setup_only==1)continue;
      nplot3dinfo++;
      continue;

    }
/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OPEN_VENT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OPEN_VENT") == 1||Match(buffer,"CLOSE_VENT")==1||
       Match(buffer, "OPEN_CVENT") == 1 || Match(buffer, "CLOSE_CVENT") == 1){
      meshdata *meshi;
      int len;
      int showvent, blocknumber, tempval, isvent;

      do_pass2=1;
      showvent=1;
      isvent = 0;
      if(Match(buffer, "CLOSE_VENT") == 1 || Match(buffer, "OPEN_VENT") == 1)isvent = 1;
      if(Match(buffer,"CLOSE_VENT") == 1 || Match(buffer, "CLOSE_CVENT") == 1)showvent = 0;
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      TrimBack(buffer);
      len=strlen(buffer);
      if(showvent==1){
        if(len>10){
          sscanf(buffer+10,"%i",&blocknumber);
          blocknumber--;
          if(blocknumber<0)blocknumber=0;
          if(blocknumber>nmeshes-1)blocknumber=nmeshes-1;
        }
      }
      else{
        if(len>11){
          sscanf(buffer+11,"%i",&blocknumber);
          blocknumber--;
          if(blocknumber<0)blocknumber=0;
          if(blocknumber>nmeshes-1)blocknumber=nmeshes-1;
        }
      }
      meshi=meshinfo + blocknumber;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&tempval,&time_local);
      tempval--;
      if(tempval<0)continue;
      if(isvent == 1){
        if(meshi->ventinfo == NULL || tempval >= meshi->nvents)continue;
      }
      else{
        if(meshi->cventinfo == NULL || tempval >= meshi->ncvents)continue;
      }
      if(isvent == 1){
        ventdata *vi;

        vi = GetCloseVent(meshi, tempval);
        vi->nshowtime++;
      }
      else{
        cventdata *cvi;

        cvi = meshi->cventinfo + tempval;
        cvi->showtimelist = NULL;
        cvi->nshowtime++;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SHOW_OBST ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SHOW_OBST") == 1||Match(buffer,"HIDE_OBST")==1){
      meshdata *meshi;
      int blocknumber,blocktemp,tempval;
      blockagedata *bc;

      do_pass2=1;
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(strlen(buffer)>10){
        sscanf(buffer,"%s %i",buffer2,&blocktemp);
        if(blocktemp>0&&blocktemp<=nmeshes)blocknumber = blocktemp-1;
      }
      meshi=meshinfo + blocknumber;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&tempval,&time_local);
      tempval--;
      if(tempval<0||tempval>=meshi->nbptrs)continue;
      bc=meshi->blockageinfoptrs[tempval];
      bc->nshowtime++;
      continue;
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ DEVICE_ACT +++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"DEVICE_ACT") == 1){
      devicedata *devicei;
      int idevice;
      float act_time;
      int act_state;

      do_pass2=1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f %i",&idevice,&act_time,&act_state);
      idevice--;
      if(idevice>=0&&idevice<ndeviceinfo){
        devicei = deviceinfo + idevice;
        devicei->act_time=act_time;
        devicei->nstate_changes++;
      }

      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ HEAT_ACT +++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"HEAT_ACT") == 1){
      meshdata *meshi;
      int blocknumber,blocktemp;
      int nn;

      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(strlen(buffer)>9){
        sscanf(buffer,"%s %i",buffer2,&blocktemp);
        if(blocktemp>0&&blocktemp<=nmeshes)blocknumber = blocktemp-1;
      }
      meshi=meshinfo + blocknumber;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&nn,&time_local);
      if(meshi->theat!=NULL && nn>=1 && nn <= meshi->nheat){
        int idev;
        int count=0;

        meshi->theat[nn-1]=time_local;
        for(idev=0;idev<ndeviceinfo;idev++){
          devicedata *devicei;

          devicei = deviceinfo + idev;
          if(devicei->type==DEVICE_HEAT){
            count++;
            if(nn==count){
              devicei->act_time=time_local;
              break;
            }
          }
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SPRK_ACT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SPRK_ACT") == 1){
      meshdata *meshi;
      int blocknumber,blocktemp;
      int nn;

      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(strlen(buffer)>9){
        sscanf(buffer,"%s %i",buffer2,&blocktemp);
        if(blocktemp>0&&blocktemp<=nmeshes)blocknumber = blocktemp-1;
      }
      meshi=meshinfo + blocknumber;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&nn,&time_local);
      if(meshi->tspr!=NULL && nn <= meshi->nspr && nn > 0){
        int idev;
        int count=0;

        meshi->tspr[nn-1]=time_local;

        for(idev=0;idev<ndeviceinfo;idev++){
          devicedata *devicei;

          devicei = deviceinfo + idev;
          if(devicei->type==DEVICE_SPRK){
            count++;
            if(nn==count){
              devicei->act_time=time_local;
              break;
            }
          }
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SMOD_ACT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SMOD_ACT") == 1){
      int idev;
      int count=0;
      int nn;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&nn,&time_local);
      for(idev=0;idev<ndeviceinfo;idev++){
        devicedata *devicei;

        devicei = deviceinfo + idev;
        if(devicei->type==DEVICE_SMOKE){
          count++;
          if(nn==count){
            devicei->act_time=time_local;
            break;
          }
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ MINMAXPL3D +++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"MINMAXPL3D") == 1){
      minmaxpl3d=1;
      do_pass2=1;
      continue;
    }
  }

  // ------------------------------- pass 1 dynamic - end ------------------------------------

  if(nplot3dinfo>0){
    if(plot3dinfo==NULL){
      NewMemory((void **)&plot3dinfo,nplot3dinfo*sizeof(plot3ddata));
    }
    else{
      ResizeMemory((void **)&plot3dinfo,nplot3dinfo*sizeof(plot3ddata));
    }
  }
  for(i=0;i<ndeviceinfo;i++){
    devicedata *devicei;

    devicei = deviceinfo + i;
    devicei->istate_changes=0;
  }

  ioffset=0;
  REWIND(stream);

  // ------------------------------- pass 2 dynamic - start ------------------------------------

  while(do_pass2==1){
    char buffer[255],buffer2[255];

    if(FGETS(buffer,255,stream)==NULL)break;
    if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OFFSET ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OFFSET") == 1){
      ioffset++;
      continue;
    }
    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ PL3D ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"PL3D") == 1){
      plot3ddata *plot3di;
      int len,blocknumber,blocktemp;
      char *bufferptr;

      if(setup_only==1)continue;
      if(minmaxpl3d==1)do_pass3=1;
      nn_plot3d++;
      TrimBack(buffer);
      len=strlen(buffer);
      blocknumber = 0;
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(strlen(buffer)>5){
        sscanf(buffer,"%s %f %i",buffer2,&time_local,&blocktemp);
        if(blocktemp>0&&blocktemp<=nmeshes)blocknumber = blocktemp-1;
      }
      else{
        time_local=-1.0;
      }
      if(FGETS(buffer,255,stream)==NULL){
        nplot3dinfo--;
        break;
      }
      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);

      plot3di=plot3dinfo+iplot3d;
      plot3di->blocknumber=blocknumber;
      plot3di->seq_id=nn_plot3d;
      plot3di->autoload=0;
      plot3di->time=time_local;
      if(plot3di>plot3dinfo+nplot3dinfo_old-1){
        plot3di->loaded=0;
        plot3di->display=0;
      }

      NewMemory((void **)&plot3di->reg_file,(unsigned int)(len+1));
      STRCPY(plot3di->reg_file,bufferptr);

      NewMemory((void **)&plot3di->comp_file,(unsigned int)(len+4+1));
      STRCPY(plot3di->comp_file,bufferptr);
      STRCAT(plot3di->comp_file,".svz");

      if(FILE_EXISTS_CASEDIR(plot3di->comp_file)==YES){
        plot3di->compression_type=COMPRESSED_ZLIB;
        plot3di->file=plot3di->comp_file;
      }
      else{
        plot3di->compression_type=UNCOMPRESSED;
        plot3di->file=plot3di->reg_file;
      }
      //disable compression for now
      plot3di->compression_type=UNCOMPRESSED;
      plot3di->file=plot3di->reg_file;

      if(fast_startup==1||FILE_EXISTS_CASEDIR(plot3di->file)==YES){
        int n;

        plot3di->u = -1;
        plot3di->v = -1;
        plot3di->w = -1;
        for(n = 0;n<5;n++){
          if(ReadLabels(&plot3di->label[n], stream, NULL)==2)return;
          if(STRCMP(plot3di->label[n].shortlabel, "U-VEL")==0){
            plot3di->u = n;
          }
          if(STRCMP(plot3di->label[n].shortlabel, "V-VEL")==0){
            plot3di->v = n;
          }
          if(STRCMP(plot3di->label[n].shortlabel, "W-VEL")==0){
            plot3di->w = n;
          }
        }
        if(plot3di->u>-1||plot3di->v>-1||plot3di->w>-1){
          plot3di->nvars = mxplot3dvars;
        }
        else{
          plot3di->nvars = 5;
        }
        if(NewMemory((void **)&plot3di->label[5].longlabel, 6)==0)return;
        if(NewMemory((void **)&plot3di->label[5].shortlabel, 6)==0)return;
        if(NewMemory((void **)&plot3di->label[5].unit, 4)==0)return;

        STRCPY(plot3di->label[5].longlabel, "Speed");
        STRCPY(plot3di->label[5].shortlabel, "Speed");
        STRCPY(plot3di->label[5].unit, "m/s");

        STRCPY(plot3di->longlabel, "");
        for(n = 0;n<5;n++){
          STRCAT(plot3di->longlabel, plot3di->label[n].shortlabel);
          if(n!=4)STRCAT(plot3di->longlabel, ", ");
        }

        iplot3d++;
      }
      else{
        int n;

        for(n = 0;n<5;n++){
          if(ReadLabels(&plot3di->label[n], stream, NULL)==2)return;
        }
        nplot3dinfo--;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OPEN_VENT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OPEN_VENT") == 1||Match(buffer,"CLOSE_VENT")==1||
       Match(buffer, "OPEN_CVENT") == 1 || Match(buffer, "CLOSE_CVENT") == 1){
      meshdata *meshi;
      int len,showvent,blocknumber,tempval,isvent;

      showvent=1;
      isvent = 0;
      if(Match(buffer, "CLOSE_VENT") == 1 || Match(buffer, "OPEN_VENT") == 1)isvent = 1;
      if(Match(buffer,"CLOSE_VENT") == 1|| Match(buffer, "CLOSE_CVENT") == 1)showvent=0;
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      TrimBack(buffer);
      len=strlen(buffer);
      if(showvent==1){
        if(len>10){
          sscanf(buffer+10,"%i",&blocknumber);
          blocknumber--;
          if(blocknumber<0)blocknumber=0;
          if(blocknumber>nmeshes-1)blocknumber=nmeshes-1;
        }
      }
      else{
        if(len>11){
          sscanf(buffer+11,"%i",&blocknumber);
          blocknumber--;
          if(blocknumber<0)blocknumber=0;
          if(blocknumber>nmeshes-1)blocknumber=nmeshes-1;
        }
      }
      meshi=meshinfo + blocknumber;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&tempval,&time_local);
      tempval--;
      if(isvent == 1){
        ventdata *vi;

        if(meshi->ventinfo == NULL || tempval < 0 || tempval >= meshi->nvents)continue;
        vi = GetCloseVent(meshi, tempval);
        if(vi->showtime == NULL){
          NewMemory((void **)&vi->showtime, (vi->nshowtime + 1) * sizeof(float));
          NewMemory((void **)&vi->showhide, (vi->nshowtime + 1) * sizeof(unsigned char));
          vi->nshowtime = 1;
          vi->showtime[0] = 0.0;
          vi->showhide[0] = 1;
        }
        if(showvent == 1){
          vi->showhide[vi->nshowtime] = 1;
        }
        else{
          vi->showhide[vi->nshowtime] = 0;
        }
        vi->showtime[vi->nshowtime++] = time_local;
      }
      else{
        cventdata *cvi;

        if(meshi->cventinfo == NULL || tempval < 0 || tempval >= meshi->ncvents)continue;
        cvi = meshi->cventinfo + tempval;
        if(cvi->showtime == NULL){
          NewMemory((void **)&cvi->showtime, (cvi->nshowtime + 1) * sizeof(float));
          NewMemory((void **)&cvi->showhide, (cvi->nshowtime + 1) * sizeof(unsigned char));
          cvi->nshowtime = 1;
          cvi->showtime[0] = 0.0;
          cvi->showhide[0] = 1;
        }
        if(showvent == 1){
          cvi->showhide[cvi->nshowtime] = 1;
        }
        else{
          cvi->showhide[cvi->nshowtime] = 0;
        }
        cvi->showtime[cvi->nshowtime++] = time_local;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SHOW_OBST ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SHOW_OBST") == 1||Match(buffer,"HIDE_OBST")==1){
      meshdata *meshi;
      int blocknumber,tempval,showobst,blocktemp;
      blockagedata *bc;

      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(strlen(buffer)>10){
        sscanf(buffer,"%s %i",buffer2,&blocktemp);
        if(blocktemp>0&&blocktemp<=nmeshes)blocknumber = blocktemp-1;
      }
      showobst=0;
      if(Match(buffer,"SHOW_OBST") == 1)showobst=1;
      meshi=meshinfo + blocknumber;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f",&tempval,&time_local);
      tempval--;
      if(tempval<0||tempval>=meshi->nbptrs)continue;
      bc=meshi->blockageinfoptrs[tempval];

      if(bc->showtime==NULL){
        if(time_local!=0.0)bc->nshowtime++;
        NewMemory((void **)&bc->showtime,bc->nshowtime*sizeof(float));
        NewMemory((void **)&bc->showhide,bc->nshowtime*sizeof(unsigned char));
        bc->nshowtime=0;
        if(time_local!=0.0){
          bc->nshowtime=1;
          bc->showtime[0]=0.0;
          if(showobst==1){
            bc->showhide[0]=0;
          }
          else{
            bc->showhide[0]=1;
          }
        }
      }
      bc->nshowtime++;
      if(showobst==1){
        bc->showhide[bc->nshowtime-1]=1;
      }
      else{
        bc->showhide[bc->nshowtime-1]=0;
      }
      bc->showtime[bc->nshowtime-1]=time_local;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ DEVICE_ACT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"DEVICE_ACT") == 1){
      devicedata *devicei;
      int idevice;
      float act_time;
      int act_state=1;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f %i",&idevice,&act_time,&act_state);
      idevice--;
      if(idevice>=0&&idevice<ndeviceinfo){
        int istate;

        devicei = deviceinfo + idevice;
        devicei->act_time=act_time;
        if(devicei->act_times==NULL){
          devicei->nstate_changes++;
          NewMemory((void **)&devicei->act_times,devicei->nstate_changes*sizeof(int));
          NewMemory((void **)&devicei->state_values,devicei->nstate_changes*sizeof(int));
          devicei->act_times[0]=0.0;
          devicei->state_values[0]=devicei->state0;
          devicei->istate_changes++;
        }
        istate = devicei->istate_changes++;
        devicei->act_times[istate]=act_time;
        devicei->state_values[istate]=act_state;
      }
      continue;
    }

  }

  // ------------------------------- pass 2 dynamic - end ------------------------------------

  REWIND(stream);

  // ------------------------------- pass 3 dynamic - start ------------------------------------

  while(do_pass3==1){
    char buffer[255];

    if(FGETS(buffer,255,stream)==NULL)break;
    if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ MINMAXPL3D +++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"MINMAXPL3D") == 1){
      char *file_ptr, file2[1024];
      float valmin[5], valmax[5];
      float percentile_min[5], percentile_max[5];

      FGETS(buffer,255,stream);
      strcpy(file2,buffer);
      file_ptr = file2;
      TrimBack(file2);
      file_ptr = TrimFront(file2);

      for(i=0;i<5;i++){
        FGETS(buffer,255,stream);
        sscanf(buffer,"%f %f %f %f",valmin +i,valmax+i, percentile_min+i,percentile_max+i);
      }

      for(i=0;i<nplot3dinfo;i++){
        plot3ddata *plot3di;

        plot3di = plot3dinfo + i;
        if(strcmp(file_ptr,plot3di->file)==0){
          int j;

          for(j=0;j<5;j++){
            plot3di->diff_valmin[j]=percentile_min[j];
            plot3di->diff_valmax[j]=percentile_max[j];
          }
          break;
        }
      }
      continue;
    }
  }
  FCLOSE(stream);
  UpdatePlot3dMenuLabels();
  InitPlot3dTimeList();
  UpdateTimes();
}


/* ------------------ GetLabels ------------------------ */

void GetLabels(char *buffer, int kind, char **label1, char **label2, char prop_buffer[255]){
  char *tok0, *tok1, *tok2;

  tok0 = NULL;
  tok1 = NULL;
  tok2 = NULL;
  tok0 = strtok(buffer, "%");
  if(tok0 != NULL)tok1 = strtok(NULL, "%");
  if(tok1 != NULL)tok2 = strtok(NULL, "%");
  if(tok1 != NULL){
    TrimBack(tok1);
    tok1 = TrimFront(tok1);
    if(strlen(tok1) == 0)tok1 = NULL;
  }
  if(tok2 != NULL){
    TrimBack(tok2);
    tok2 = TrimFront(tok2);
    if(strlen(tok2) == 0)tok2 = NULL;
  }
  if(label2 != NULL){
    if(tok2 == NULL&&kind == HUMANS){
      strcpy(prop_buffer, "Human_props(default)");
      *label2 = prop_buffer;
    }
    else{
      *label2 = tok2;
    }
  }
  if(label1 != NULL)*label1 = tok1;
}

/* ------------------ GetPropID ------------------------ */

propdata *GetPropID(char *prop_id){
  int i;

  if(propinfo == NULL || prop_id == NULL || strlen(prop_id) == 0)return NULL;
  for(i = 0; i < npropinfo; i++){
    propdata *propi;

    propi = propinfo + i;

    if(strcmp(propi->label, prop_id) == 0)return propi;
  }
  return NULL;
}

/* ----------------------- InitDevice ----------------------------- */

void InitDevice(devicedata *devicei, float *xyz, int is_beam, float *xyz1, float *xyz2, float *xyzn, int state0, int nparams, float *params, char *labelptr){
  float norm;
  int i;

  devicei->nvals = 0;
  devicei->filetype = -1;
  devicei->in_zone_csv = 0;
  devicei->in_devc_csv = 0;
  devicei->labelptr = devicei->label;
  devicei->color = NULL;
  devicei->line_width = 1.0;
  if(labelptr != NULL){
    strcpy(devicei->label, labelptr);
  }
  if(STRCMP(devicei->object->label, "plane") == 0){
    float color[4];

    NewMemory((void **)&devicei->plane_surface, nmeshes * sizeof(isosurface *));
    for(i = 0; i < nmeshes; i++){
      NewMemory((void **)&devicei->plane_surface[i], sizeof(isosurface));
    }
    if(nparams >= 3){
      color[0] = params[0];
      color[1] = params[1];
      color[2] = params[2];
      color[3] = 1.0;
      devicei->color = GetColorPtr(color);
    }
    if(nparams >= 4){
      devicei->line_width = params[3];
    }
  }
  else{
    devicei->plane_surface = NULL;
  }
  if(xyz != NULL){
    devicei->xyz[0] = xyz[0];
    devicei->xyz[1] = xyz[1];
    devicei->xyz[2] = xyz[2];
  }
  if(xyz1 != NULL){
    devicei->xyz1[0] = xyz1[0];
    devicei->xyz1[1] = xyz1[1];
    devicei->xyz1[2] = xyz1[2];
  }
  if(xyz2 != NULL){
    devicei->xyz2[0] = xyz2[0];
    devicei->xyz2[1] = xyz2[1];
    devicei->xyz2[2] = xyz2[2];
  }
  if(is_beam == 1)have_beam = 1;
  devicei->is_beam = is_beam;
  norm = sqrt(xyzn[0] * xyzn[0] + xyzn[1] * xyzn[1] + xyzn[2] * xyzn[2]);
  if(norm != 0.0){
    devicei->xyznorm[0] = xyzn[0] / norm;
    devicei->xyznorm[1] = xyzn[1] / norm;
    devicei->xyznorm[2] = xyzn[2] / norm;
  }
  else{
    devicei->xyznorm[0] = 0.0;
    devicei->xyznorm[1] = 0.0;
    devicei->xyznorm[2] = 1.0;
  }
  devicei->times = NULL;
  devicei->vals = NULL;
  devicei->nstate_changes = 0;
  devicei->istate_changes = 0;
  devicei->act_times = NULL;
  devicei->state_values = NULL;
  devicei->showstatelist = NULL;
  devicei->act_time = -1.0;
  devicei->device_mesh = NULL;
  devicei->state0 = state0;
  devicei->nparams = nparams;
  devicei->params = params;
  devicei->ival = 0;
  if(nparams > 0 && params != NULL){
    for(i = 0; i < nparams; i++){
      devicei->params[i] = params[i];
    }
  }
}

/* ------------------ ParseDevicekeyword ------------------------ */

void ParseDevicekeyword(BFILE *stream, devicedata *devicei){
  float xyz[3]={0.0,0.0,0.0}, xyzn[3]={0.0,0.0,0.0};
  float xyz1[3] = { 0.0,0.0,0.0 }, xyz2[3] = { 0.0,0.0,0.0 };
  int state0=0;
  int nparams=0, nparams_textures=0;
  char *labelptr, *prop_id;
  char prop_buffer[255];
  char buffer[255],*buffer3;
  int i;
  char *tok1, *tok2, *tok3;
  int is_beam=0;

  devicei->type=DEVICE_DEVICE;
  FGETS(buffer,255,stream);
  TrimCommas(buffer);

  tok1=strtok(buffer,"%");
  tok1=TrimFrontBack(tok1);

  tok2=strtok(NULL,"%");
  tok2=TrimFrontBack(tok2);

  tok3=strtok(NULL,"%");
  tok3=TrimFrontBack(tok3);

  strcpy(devicei->quantity,"");
  if(tok2!=NULL){
    strcpy(devicei->quantity,tok2);
  }

  if(strlen(tok1)>=4&&strncmp(tok1, "null",4)==0){
    strcpy(devicei->label, "null");
  }
  else{
    strcpy(devicei->label, tok1);
  }
  devicei->object = GetSmvObjectType(tok1,missing_device);
  if(devicei->object==missing_device&&tok3!=NULL){
    devicei->object = GetSmvObjectType(tok3,missing_device);
  }
  if(devicei->object == missing_device)have_missing_objects = 1;
  devicei->params=NULL;
  devicei->times=NULL;
  devicei->vals=NULL;
  FGETS(buffer,255,stream);
  TrimCommas(buffer);

  sscanf(buffer,"%f %f %f %f %f %f %i %i %i",
    xyz,xyz+1,xyz+2,xyzn,xyzn+1,xyzn+2,&state0,&nparams,&nparams_textures);

  labelptr = strchr(buffer, '#'); // read in coordinates of beam detector
  if(labelptr != NULL){
    sscanf(labelptr + 1, "%f %f %f %f %f %f", xyz1, xyz1 + 1, xyz1 + 2, xyz2, xyz2 + 1, xyz2 + 2);
    if(strcmp(devicei->quantity, "PATH OBSCURATION") == 0 ||
      strcmp(devicei->quantity, "TRANSMISSION") == 0){
      is_beam = 1;
    }
  }
  devicei->is_beam = is_beam;

  GetLabels(buffer,-1,&prop_id,NULL,prop_buffer);
  devicei->prop=GetPropID(prop_id);
  if(prop_id!=NULL&&devicei->prop!=NULL&&devicei->prop->smv_object!=NULL){
    devicei->object=devicei->prop->smv_object;
  }
  else{
    NewMemory((void **)&devicei->prop,sizeof(propdata));
    InitProp(devicei->prop,1,devicei->label);
    devicei->prop->smv_object=devicei->object;
    devicei->prop->smv_objects[0]=devicei->prop->smv_object;
  }
  if(nparams_textures<0)nparams_textures=0;
  if(nparams_textures>1)nparams_textures=1;
  devicei->ntextures=nparams_textures;
  if(nparams_textures>0){
     NewMemory((void **)&devicei->textureinfo,sizeof(texturedata));
  }
  else{
    devicei->textureinfo=NULL;
    devicei->texturefile=NULL;
  }

  labelptr=strchr(buffer,'%');
  if(labelptr!=NULL){
    TrimBack(labelptr);
    if(strlen(labelptr)>1){
      labelptr++;
      labelptr=TrimFront(labelptr);
      if(strlen(labelptr)==0)labelptr=NULL;
    }
    else{
      labelptr=NULL;
    }
  }

  if(nparams<=0){
    InitDevice(devicei,xyz,is_beam,xyz1,xyz2,xyzn,state0,0,NULL,labelptr);
  }
  else{
    float *params,*pc;
    int nsize;

    nsize = 6*((nparams-1)/6+1);
    NewMemory((void **)&params,(nsize+devicei->ntextures)*sizeof(float));
    pc=params;
    for(i=0;i<nsize/6;i++){
      FGETS(buffer,255,stream);
      TrimCommas(buffer);

      sscanf(buffer,"%f %f %f %f %f %f",pc,pc+1,pc+2,pc+3,pc+4,pc+5);
      pc+=6;
    }
    InitDevice(devicei,xyz,is_beam,xyz1,xyz2,xyzn,state0,nparams,params,labelptr);
  }
  GetElevAz(devicei->xyznorm,&devicei->dtheta,devicei->rotate_axis,NULL);
  if(nparams_textures>0){
    FGETS(buffer,255,stream);
    TrimCommas(buffer);
    TrimBack(buffer);
    buffer3=TrimFront(buffer);
    NewMemory((void **)&devicei->texturefile,strlen(buffer3)+1);
    strcpy(devicei->texturefile,buffer3);
  }
}

/* ------------------ ParseDevicekeyword2 ------------------------ */

void ParseDevicekeyword2(FILE *stream, devicedata *devicei){
  float xyz[3] = {0.0,0.0,0.0}, xyzn[3] = {0.0,0.0,0.0};
  float xyz1[3] = {0.0,0.0,0.0}, xyz2[3] = {0.0,0.0,0.0};
  int state0 = 0;
  int nparams = 0, nparams_textures = 0;
  char *labelptr, *prop_id;
  char prop_buffer[255];
  char buffer[255], *buffer3;
  int i;
  char *tok1, *tok2, *tok3;
  int is_beam = 0;

  devicei->type = DEVICE_DEVICE;
  fgets(buffer, 255, stream);
  TrimCommas(buffer);

  tok1 = strtok(buffer, "%");
  tok1 = TrimFrontBack(tok1);

  tok2 = strtok(NULL, "%");
  tok2 = TrimFrontBack(tok2);

  tok3 = strtok(NULL, "%");
  tok3 = TrimFrontBack(tok3);

  strcpy(devicei->quantity, "");
  if(tok2!=NULL){
    strcpy(devicei->quantity, tok2);
  }

  if(strlen(tok1)>=4&&strncmp(tok1, "null", 4)==0){
    strcpy(devicei->label, "null");
  }
  else{
    strcpy(devicei->label, tok1);
  }
  devicei->object = GetSmvObjectType(tok1, missing_device);
  if(devicei->object==missing_device&&tok3!=NULL){
    devicei->object = GetSmvObjectType(tok3, missing_device);
  }
  if(devicei->object==missing_device)have_missing_objects = 1;
  devicei->params = NULL;
  devicei->times = NULL;
  devicei->vals = NULL;
  fgets(buffer, 255, stream);
  TrimCommas(buffer);

  sscanf(buffer, "%f %f %f %f %f %f %i %i %i",
    xyz, xyz+1, xyz+2, xyzn, xyzn+1, xyzn+2, &state0, &nparams, &nparams_textures);

  labelptr = strchr(buffer, '#'); // read in coordinates of a possible beam detector
  if(labelptr!=NULL){
    sscanf(labelptr+1, "%f %f %f %f %f %f", xyz1, xyz1+1, xyz1+2, xyz2, xyz2+1, xyz2+2);
    if(strcmp(devicei->quantity, "PATH OBSCURATION") == 0||
       strcmp(devicei->quantity, "TRANSMISSION") == 0){
      is_beam = 1;
    }
  }
  devicei->is_beam = is_beam;

  GetLabels(buffer, -1, &prop_id, NULL, prop_buffer);
  devicei->prop = GetPropID(prop_id);
  if(prop_id!=NULL&&devicei->prop!=NULL&&devicei->prop->smv_object!=NULL){
    devicei->object = devicei->prop->smv_object;
  }
  else{
    NewMemory((void **)&devicei->prop, sizeof(propdata));
    InitProp(devicei->prop, 1, devicei->label);
    devicei->prop->smv_object = devicei->object;
    devicei->prop->smv_objects[0] = devicei->prop->smv_object;
  }
  if(nparams_textures<0)nparams_textures = 0;
  if(nparams_textures>1)nparams_textures = 1;
  devicei->ntextures = nparams_textures;
  if(nparams_textures>0){
    NewMemory((void **)&devicei->textureinfo, sizeof(texturedata));
  }
  else{
    devicei->textureinfo = NULL;
    devicei->texturefile = NULL;
  }

  labelptr = strchr(buffer, '%');
  if(labelptr!=NULL){
    TrimBack(labelptr);
    if(strlen(labelptr)>1){
      labelptr++;
      labelptr = TrimFront(labelptr);
      if(strlen(labelptr)==0)labelptr = NULL;
    }
    else{
      labelptr = NULL;
    }
  }

  if(nparams<=0){
    InitDevice(devicei, xyz, is_beam, xyz1, xyz2, xyzn, state0, 0, NULL, labelptr);
  }
  else{
    float *params, *pc;
    int nsize;

    nsize = 6*((nparams-1)/6+1);
    NewMemory((void **)&params, (nsize+devicei->ntextures)*sizeof(float));
    pc = params;
    for(i = 0;i<nsize/6;i++){
      fgets(buffer, 255, stream);
      TrimCommas(buffer);

      sscanf(buffer, "%f %f %f %f %f %f", pc, pc+1, pc+2, pc+3, pc+4, pc+5);
      pc += 6;
    }
    InitDevice(devicei, xyz, is_beam, xyz1, xyz2, xyzn, state0, nparams, params, labelptr);
  }
  GetElevAz(devicei->xyznorm, &devicei->dtheta, devicei->rotate_axis, NULL);
  if(nparams_textures>0){
    fgets(buffer, 255, stream);
    TrimCommas(buffer);
    TrimBack(buffer);
    buffer3 = TrimFront(buffer);
    NewMemory((void **)&devicei->texturefile, strlen(buffer3)+1);
    strcpy(devicei->texturefile, buffer3);
  }
}

/* ------------------ GetInpf ------------------------ */

int GetInpf(char *file, char *file2){
  FILE *stream=NULL,*stream1=NULL,*stream2=NULL;
  char buffer[255],*bufferptr;
  int len;

  if(file==NULL)return 1;
  stream1=fopen(file,"r");
  if(stream1==NULL)return 1;
  if(file2!=NULL){
    stream2=fopen(file2,"r");
    if(stream2==NULL){
      fclose(stream1);
      return 1;
    }
  }
  stream=stream1;
  for(;;){
    if(feof(stream)!=0){
      BREAK2;
    }
    if(fgets(buffer,255,stream)==NULL){
      BREAK2;
    }
    if(strncmp(buffer," ",1)==0)continue;
    if(Match(buffer,"INPF") == 1){
      if(fgets(buffer,255,stream)==NULL){
        BREAK2;
      }
      bufferptr=TrimFrontBack(buffer);

      len=strlen(bufferptr);
      FREEMEMORY(fds_filein);
      if(NewMemory((void **)&fds_filein,(unsigned int)(len+1))==0)return 2;
      STRCPY(fds_filein,bufferptr);
      if(FILE_EXISTS_CASEDIR(fds_filein)==NO){
        FreeMemory(fds_filein);
      }

      if(chidfilebase==NULL){
        char *chidptr=NULL;
        char buffer_chid[1024];

        if(fds_filein!=NULL)chidptr=GetChid(fds_filein,buffer_chid);
        if(chidptr!=NULL){
          NewMemory((void **)&chidfilebase,(unsigned int)(strlen(chidptr)+1));
          STRCPY(chidfilebase,chidptr);
        }
      }
      if(chidfilebase!=NULL){
        NewMemory((void **)&hrr_csv_filename,(unsigned int)(strlen(chidfilebase)+8+1));
        STRCPY(hrr_csv_filename,chidfilebase);
        STRCAT(hrr_csv_filename,"_hrr.csv");
        if(FILE_EXISTS_CASEDIR(hrr_csv_filename)==NO){
          FREEMEMORY(hrr_csv_filename);
        }

        NewMemory((void **)&devc_csv_filename,(unsigned int)(strlen(chidfilebase)+9+1));
        STRCPY(devc_csv_filename,chidfilebase);
        STRCAT(devc_csv_filename,"_devc.csv");
        if(FILE_EXISTS_CASEDIR(devc_csv_filename)==NO){
          FREEMEMORY(devc_csv_filename);
        }

        NewMemory((void **)&exp_csv_filename,(unsigned int)(strlen(chidfilebase)+8+1));
        STRCPY(exp_csv_filename,chidfilebase);
        STRCAT(exp_csv_filename,"_exp.csv");
        if(FILE_EXISTS_CASEDIR(exp_csv_filename)==NO){
          FREEMEMORY(exp_csv_filename);
        }
      }
      continue;
    }
  }
  fclose(stream);
  return 0;
}

/* ------------------ InitTextures ------------------------ */

void InitTextures(void){
  // get texture filename from SURF and device info
  int i;

  ntextures = 0;
  for(i=0;i<nsurfinfo;i++){
    surfdata *surfi;
    texturedata *texti;
    int len;

    surfi = surfinfo + i;
    if(surfi->texturefile==NULL)continue;
    texti = textureinfo + ntextures;
    len = strlen(surfi->texturefile);
    NewMemory((void **)&texti->file,(len+1)*sizeof(char));
    strcpy(texti->file,surfi->texturefile);
    texti->loaded=0;
    texti->used=0;
    texti->display=0;
    ntextures++;
    surfi->textureinfo=textureinfo+ntextures-1;
  }

  for(i=0;i<ndevice_texture_list;i++){
    char *texturefile;
    texturedata *texti;
    int len;

    texturefile = device_texture_list[i];
    texti = textureinfo + ntextures;
    len = strlen(texturefile);
    NewMemory((void **)&texti->file,(len+1)*sizeof(char));
    device_texture_list_index[i]=ntextures;
    strcpy(texti->file,texturefile);
    texti->loaded=0;
    texti->used=0;
    texti->display=0;
    ntextures++;
  }

  // check to see if texture files exist .
  // If so, then convert to OpenGL format

  for(i=0;i<ntextures;i++){
    unsigned char *floortex;
    int texwid, texht;
    texturedata *texti;
    int dup_texture;
    int j;

    texti = textureinfo + i;
    texti->loaded=0;
    if(texti->file==NULL){
      continue;
    }
    dup_texture=0;
    for(j=0;j<i;j++){
      texturedata *textj;

      textj = textureinfo + j;
      if(textj->loaded==0)continue;
      if(strcmp(texti->file,textj->file)==0){
        texti->name=textj->name;
        texti->loaded=1;
        dup_texture=1;
      }
    }
    if(dup_texture==1){
      continue;
    }
    if(use_graphics==1){
      int errorcode;
      char *filename;

      CheckMemory;
      filename=strrchr(texti->file,*dirseparator);
      if(filename!=NULL){
        filename++;
      }
      else{
        filename=texti->file;
      }
      glGenTextures(1,&texti->name);
      glBindTexture(GL_TEXTURE_2D,texti->name);
      floortex=ReadPicture(texti->file,&texwid,&texht,0);
      if(floortex==NULL){
        PRINTF("***Error: Texture %s failed to load\n", filename);
        continue;
      }
      errorcode=gluBuild2DMipmaps(GL_TEXTURE_2D,4, texwid, texht, GL_RGBA, GL_UNSIGNED_BYTE, floortex);
      if(errorcode!=0){
        FREEMEMORY(floortex);
        PRINTF("***Error: Texture %s failed to load\n", filename);
        continue;
      }
      FREEMEMORY(floortex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      texti->loaded=1;
    }
  }

  CheckMemory;
  if(ntextures==0){
    FREEMEMORY(textureinfo);
  }

  // define colorbar textures

 // glActiveTexture(GL_TEXTURE0);
  glGenTextures(1,&texture_colorbar_id);
  glBindTexture(GL_TEXTURE_1D,texture_colorbar_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  if(gpuactive==1){
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  }
  else{
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  }
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,256,0,GL_RGBA,GL_FLOAT,rgb_full);

  glGenTextures(1, &terrain_colorbar_id);
  glBindTexture(GL_TEXTURE_1D, terrain_colorbar_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, rgb_terrain2);

  glGenTextures(1,&texture_slice_colorbar_id);
  glBindTexture(GL_TEXTURE_1D,texture_slice_colorbar_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,256,0,GL_RGBA,GL_FLOAT,rgb_slice);

  glGenTextures(1,&texture_patch_colorbar_id);
  glBindTexture(GL_TEXTURE_1D,texture_patch_colorbar_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,256,0,GL_RGBA,GL_FLOAT,rgb_patch);

  glGenTextures(1,&texture_plot3d_colorbar_id);
  glBindTexture(GL_TEXTURE_1D,texture_plot3d_colorbar_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,256,0,GL_RGBA,GL_FLOAT,rgb_plot3d);

  glGenTextures(1,&texture_iso_colorbar_id);
  glBindTexture(GL_TEXTURE_1D,texture_iso_colorbar_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,256,0,GL_RGBA,GL_FLOAT,rgb_iso);

  glGenTextures(1,&volsmoke_colormap_id);
  glBindTexture(GL_TEXTURE_1D,volsmoke_colormap_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,MAXSMOKERGB,0,GL_RGBA,GL_FLOAT,rgb_volsmokecolormap);

  glGenTextures(1,&slicesmoke_colormap_id);
  glBindTexture(GL_TEXTURE_1D,slicesmoke_colormap_id);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#ifdef pp_GPU
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
#else
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
#endif
  glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,MAXSMOKERGB,0,GL_RGBA,GL_FLOAT,rgb_slicesmokecolormap);

  CheckMemory;

#ifdef pp_GPU
#ifdef pp_GPUDEPTH
  if(use_graphics==1){
    CreateDepthTexture();
  }
#endif
#endif

  if(autoterrain==1&&use_graphics==1){
    texturedata *tt;
    unsigned char *floortex;
    int texwid, texht;
    int errorcode;

    tt = terrain_texture;
    tt->loaded=0;
    tt->used=0;
    tt->display=0;

    glGenTextures(1,&tt->name);
    glBindTexture(GL_TEXTURE_2D,tt->name);
    floortex=NULL;
    errorcode=1;
    if(tt->file!=NULL){
      PRINTF(": %s",tt->file);
      floortex=ReadPicture(tt->file,&texwid,&texht,0);
      if(floortex==NULL)PRINTF("***Error: Texture file %s failed to load\n",tt->file);
    }
    if(floortex!=NULL){
      errorcode=gluBuild2DMipmaps(GL_TEXTURE_2D,4, texwid, texht, GL_RGBA, GL_UNSIGNED_BYTE, floortex);
      if(errorcode!=0)PRINTF("***Error: Texture file %s failed to load\n",tt->file);
    }
    FREEMEMORY(floortex);
    if(errorcode==0){
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      tt->loaded=1;
    }
  }
}

/* ------------------ UpdateBoundInfo ------------------------ */

void UpdateBoundInfo(void){
  int i,n;

  if(nisoinfo>0){
    FREEMEMORY(isoindex);
    FREEMEMORY(isobounds);
    NewMemory((void*)&isoindex,nisoinfo*sizeof(int));
    NewMemory((void*)&isobounds,nisoinfo*sizeof(boundsdata));
    niso_bounds=0;
    for(i=0;i<nisoinfo;i++){
      isodata *isoi;

      isoi = isoinfo + i;
      if(isoi->dataflag==0)continue;
      isoi->firstshort=1;
      isoi->setvalmin=0;
      isoi->setvalmax=0;
      isoi->valmin=1.0;
      isoi->valmax=0.0;
      isoindex[niso_bounds]=i;
      isobounds[niso_bounds].datalabel=isoi->color_label.shortlabel;
      isobounds[niso_bounds].setvalmin=0;
      isobounds[niso_bounds].setvalmax=0;
      isobounds[niso_bounds].valmin=1.0;
      isobounds[niso_bounds].valmax=0.0;
      isobounds[niso_bounds].setchopmax=0;
      isobounds[niso_bounds].setchopmin=0;
      isobounds[niso_bounds].chopmax=0.0;
      isobounds[niso_bounds].chopmin=1.0;
      isobounds[niso_bounds].label=&isoi->color_label;
      niso_bounds++;
      for(n=0;n<i;n++){
        isodata *ison;

        ison = isoinfo + n;
        if(ison->dataflag==0)continue;
        if(strcmp(isoi->color_label.shortlabel,ison->color_label.shortlabel)==0){
          isoi->firstshort=0;
          niso_bounds--;
          break;
        }
      }
    }
  }

  if(nsliceinfo>0){
    FREEMEMORY(slicebounds);
    NewMemory((void*)&slicebounds,nsliceinfo*sizeof(boundsdata));
    nslice_type=0;
    for(i=0;i<nsliceinfo;i++){
      slicedata *slicei;

      slicei = sliceinfo + i;
      slicei->firstshort=1;
      slicei->valmin=1.0;
      slicei->valmax=0.0;
      slicei->setvalmin=0;
      slicei->setvalmax=0;
      slicebounds[nslice_type].datalabel=slicei->label.shortlabel;
      slicebounds[nslice_type].setvalmin=0;
      slicebounds[nslice_type].setvalmax=0;
      slicebounds[nslice_type].valmin=1.0;
      slicebounds[nslice_type].valmax=0.0;
      slicebounds[nslice_type].chopmax=0.0;
      slicebounds[nslice_type].chopmin=1.0;
      slicebounds[nslice_type].setchopmax=0;
      slicebounds[nslice_type].setchopmin=0;
      slicebounds[nslice_type].line_contour_min=0.0;
      slicebounds[nslice_type].line_contour_max=1.0;
      slicebounds[nslice_type].line_contour_num=1;
      nslice_type++;
      for(n=0;n<i;n++){
        slicedata *slicen;

        slicen = sliceinfo + n;
        if(strcmp(slicei->label.shortlabel,slicen->label.shortlabel)==0){
          slicei->firstshort=0;
          nslice_type--;
          break;
        }
      }
    }
  }
  canshow_threshold=0;
  if(npatchinfo>0){
    npatch2=0;
    FREEMEMORY(patchlabellist);
    FREEMEMORY(patchlabellist_index);
    NewMemory((void **)&patchlabellist,npatchinfo*sizeof(char *));
    NewMemory((void **)&patchlabellist_index,npatchinfo*sizeof(int));
    for(i=0;i<npatchinfo;i++){
      patchdata *patchi;

      patchi = patchinfo + i;
      patchi->firstshort=1;
      patchi->valmin=1.0;
      patchi->valmax=0.0;
      patchi->setvalmin=0;
      patchi->setvalmax=0;
      if(strncmp(patchi->label.shortlabel,"temp",4)==0||
         strncmp(patchi->label.shortlabel,"TEMP",4)==0){
        canshow_threshold=1;
      }
      patchlabellist[npatch2]=patchi->label.shortlabel;
      patchlabellist_index[npatch2]=i;
      npatch2++;
      for(n=0;n<i;n++){
        patchdata *patchn;

        patchn = patchinfo + n;
        if(strcmp(patchi->label.shortlabel,patchn->label.shortlabel)==0){
          patchi->firstshort=0;
          npatch2--;
          break;
        }
      }
    }
  }
  UpdateChar();
}

/* ------------------ UpdateEndianInfo ------------------------ */

void UpdateEndianInfo(void){
  if(setendian==0){
    if(Match(LESsystem,"AIX")==1||Match(LESsystem,"SGI")==1||Match(LESendian,"b")==1||Match(LESendian,"B")==1){
      endian_data=1;
    }
    if(Match(LESsystem,"DVF")==1||Match(LESendian,"l")==1||Match(LESendian,"L")==1){
      endian_data=0;
    }
    endian_smv = endian_data;
  }

#ifndef WIN32
  if(endian_smv!=GetEndian()){
    fprintf(stderr,"*** Warning: Smokeview is running on a ");
    if(GetEndian()==1){
      fprintf(stderr," little endian computer\n");
    }
    else{
      fprintf(stderr," big endian computer\n");
    }
    fprintf(stderr,"    but the data being visualized was generated on a ");
    if(endian_smv==1){
      fprintf(stderr," little endian computer\n");
    }
    else{
      fprintf(stderr," big endian computer\n");
    }
  }
#endif
}

/*
new OBST format:
i1 i2 j1 j2 k1 k2 colorindex blocktype       : if blocktype!=1&&colorindex!=-3
i1 i2 j1 j2 k1 k2 colorindex blocktype r g b : if blocktype!=1&&colorindex==-3
i1 i2 j1 j2 k1 k2 ceiling texture blocktype [wall texture floor texture] : if blocktype==1
int colorindex, blocktype;
colorindex: -1 default color
-2 invisible
-3 use r g b color
>=0 color/color2/texture index
blocktype: 0 regular block non-textured
1 regular block textured
2 outline
3 smoothed block
r g b           colors used if colorindex==-3
*/

/* ------------------ BackupBlockage ------------------------ */

void BackupBlockage(blockagedata *bc){
  int i;

  bc->xyzORIG[0] = bc->xmin;
  bc->xyzORIG[1] = bc->xmax;
  bc->xyzORIG[2] = bc->ymin;
  bc->xyzORIG[3] = bc->ymax;
  bc->xyzORIG[4] = bc->zmin;
  bc->xyzORIG[5] = bc->zmax;
  bc->ijkORIG[0] = bc->ijk[0];
  bc->ijkORIG[1] = bc->ijk[1];
  bc->ijkORIG[2] = bc->ijk[2];
  bc->ijkORIG[3] = bc->ijk[3];
  bc->ijkORIG[4] = bc->ijk[4];
  bc->ijkORIG[5] = bc->ijk[5];
  for(i = 0; i<6; i++){
    bc->surfORIG[i] = bc->surf[i];
    bc->surf_indexORIG[i] = bc->surf_index[i];
  }
}

/* ------------------ UpdateVentOffset ------------------------ */

void UpdateVentOffset(void){
  int i;

  for(i = 0;i < nmeshes;i++){
    meshdata *meshi;

    meshi = meshinfo + i;
    meshi->vent_offset[XXX] = ventoffset_factor*(meshi->xplt[1] - meshi->xplt[0]);
    meshi->vent_offset[YYY] = ventoffset_factor*(meshi->yplt[1] - meshi->yplt[0]);
    meshi->vent_offset[ZZZ] = ventoffset_factor*(meshi->zplt[1] - meshi->zplt[0]);
  }
}

/* ------------------ UpdateMeshCoords ------------------------ */

void UpdateMeshCoords(void){
  int nn, i,n;
  float dxsbar, dysbar, dzsbar;
  int factor;
  int igrid;

  if(setPDIM==0){
    for(nn=0;nn<=current_mesh->ibar;nn++){
      current_mesh->xplt[nn]=xbar0+(float)nn*(xbar-xbar0)/(float)current_mesh->ibar;
    }
    for(nn=0;nn<=current_mesh->jbar;nn++){
      current_mesh->yplt[nn]=ybar0+(float)nn*(ybar-ybar0)/(float)current_mesh->jbar;
    }
    for(nn=0;nn<=current_mesh->kbar;nn++){
      current_mesh->zplt[nn]=zbar0+(float)nn*(zbar-zbar0)/(float)current_mesh->kbar;
    }
  }

  /* define highlighted block */

  /* add in offsets */
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;
    int ii;

    meshi=meshinfo+i;
    meshi->xyz_bar[XXX] += meshi->offset[XXX];
    meshi->xyz_bar[YYY] += meshi->offset[YYY];
    meshi->xyz_bar[ZZZ] += meshi->offset[ZZZ];
    meshi->xyz_bar0[XXX] += meshi->offset[XXX];
    meshi->xyz_bar0[YYY] += meshi->offset[YYY];
    meshi->xyz_bar0[ZZZ] += meshi->offset[ZZZ];
    {
      float dx, dy, dz;

      dx = (meshi->xyz_bar[XXX] - meshi->xyz_bar0[XXX])/meshi->ibar;
      dy = (meshi->xyz_bar[YYY] - meshi->xyz_bar0[YYY])/meshi->jbar;
      dz = (meshi->xyz_bar[ZZZ] - meshi->xyz_bar0[ZZZ])/meshi->kbar;
      meshi->cellsize=sqrt(dx*dx+dy*dy+dz*dz);
    }
    for(ii=0;ii<meshi->ibar+1;ii++){
      meshi->xplt[ii] += meshi->offset[XXX];
    }
    for(ii=0;ii<meshi->jbar+1;ii++){
      meshi->yplt[ii] += meshi->offset[YYY];
    }
    for(ii=0;ii<meshi->kbar+1;ii++){
      meshi->zplt[ii] += meshi->offset[ZZZ];
    }
    meshi->xcen+=meshi->offset[XXX];
    meshi->ycen+=meshi->offset[YYY];
    meshi->zcen+=meshi->offset[ZZZ];
  }
  for(i=1;i<nmeshes;i++){
    meshdata *meshi;

    meshi=meshinfo+i;
    if(meshi->xyz_bar0[ZZZ]!=meshinfo->xyz_bar0[ZZZ]){
      visFloor=0;
      updatefacelists=1;
      updatemenu=1;
      break;
    }
  }

  ijkbarmax=meshinfo->ibar;
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;

    meshi=meshinfo+i;

    ijkbarmax = MAX(ijkbarmax,meshi->ibar);
    ijkbarmax = MAX(ijkbarmax,meshi->jbar);
    ijkbarmax = MAX(ijkbarmax,meshi->kbar);
  }

  for(i=0;i<nmeshes;i++){
    meshdata *meshi;

    meshi=meshinfo+i;
    if(i==0){
      xbar = meshi->xyz_bar[XXX];
      ybar = meshi->xyz_bar[YYY];
      zbar = meshi->xyz_bar[ZZZ];

      xbar0 = meshi->xyz_bar0[XXX];
      ybar0 = meshi->xyz_bar0[YYY];
      zbar0 = meshi->xyz_bar0[ZZZ];
    }
    xbar = MAX(xbar, meshi->xyz_bar[XXX]);
    ybar = MAX(ybar, meshi->xyz_bar[YYY]);
    zbar = MAX(zbar, meshi->xyz_bar[ZZZ]);
    xbar0 = MIN(xbar0, meshi->xyz_bar0[XXX]);
    ybar0 = MIN(ybar0, meshi->xyz_bar0[YYY]);
    zbar0 = MIN(zbar0, meshi->xyz_bar0[ZZZ]);
  }

  factor = 256*128;
  dxsbar = (xbar-xbar0)/factor;
  dysbar = (ybar-ybar0)/factor;
  dzsbar = (zbar-zbar0)/factor;

  for(nn=0;nn<factor;nn++){
    xplts[nn]=xbar0+((float)nn+0.5)*dxsbar;
  }
  for(nn=0;nn<factor;nn++){
    yplts[nn]=ybar0+((float)nn+0.5)*dysbar;
  }
  for(nn=0;nn<factor;nn++){
    zplts[nn]=zbar0+((float)nn+0.5)*dzsbar;
  }

  /* compute scaling factors */

  {
    float dxclip, dyclip, dzclip;

    dxclip = (xbar-xbar0)/1000.0;
    dyclip = (ybar-ybar0)/1000.0;
    dzclip = (zbar-zbar0)/1000.0;
    xclip_min = xbar0-dxclip;
    yclip_min = ybar0-dyclip;
    zclip_min = zbar0-dzclip;
    xclip_max = xbar+dxclip;
    yclip_max = ybar+dyclip;
    zclip_max = zbar+dzclip;
  }

  // compute scaling factor used in NORMALIXE_X, NORMALIZE_Y, NORMALIZE_Z macros

  xyzmaxdiff=MAX(MAX(xbar-xbar0,ybar-ybar0),zbar-zbar0);

  // normalize various coordinates.

  for(nn=0;nn<factor;nn++){
    xplts[nn]=NORMALIZE_X(xplts[nn]);
  }
  for(nn=0;nn<factor;nn++){
    yplts[nn]=NORMALIZE_Y(yplts[nn]);
  }
  for(nn=0;nn<factor;nn++){
    zplts[nn]=NORMALIZE_Z(zplts[nn]);
  }

  /* rescale both global and local xbar, ybar and zbar */

  xbar0ORIG = xbar0;
  ybar0ORIG = ybar0;
  zbar0ORIG = zbar0;
  xbarORIG = xbar;
  ybarORIG = ybar;
  zbarORIG = zbar;
  xbar = NORMALIZE_X(xbar);
  ybar = NORMALIZE_Y(ybar);
  zbar = NORMALIZE_Z(zbar);
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;

    meshi=meshinfo+i;
    /* compute a local scaling factor for each block */
    meshi->xyzmaxdiff=MAX(MAX(meshi->xyz_bar[XXX]-meshi->xyz_bar0[XXX],meshi->xyz_bar[YYY]-meshi->xyz_bar0[YYY]),meshi->xyz_bar[ZZZ]-meshi->xyz_bar0[ZZZ]);

    NORMALIZE_XYZ(meshi->xyz_bar,meshi->xyz_bar);
    meshi->xcen = NORMALIZE_X(meshi->xcen);
    meshi->ycen = NORMALIZE_Y(meshi->ycen);
    meshi->zcen = NORMALIZE_Z(meshi->zcen);
  }

  for(i=0;i<noutlineinfo;i++){
    outlinedata *outlinei;
    float *x1, *x2, *yy1, *yy2, *z1, *z2;
    int j;

    outlinei = outlineinfo + i;
    x1 = outlinei->x1;
    x2 = outlinei->x2;
    yy1 = outlinei->y1;
    yy2 = outlinei->y2;
    z1 = outlinei->z1;
    z2 = outlinei->z2;
    for(j=0;j<outlinei->nlines;j++){
      x1[j]=NORMALIZE_X(x1[j]);
      x2[j]=NORMALIZE_X(x2[j]);
      yy1[j]=NORMALIZE_Y(yy1[j]);
      yy2[j]=NORMALIZE_Y(yy2[j]);
      z1[j]=NORMALIZE_Z(z1[j]);
      z2[j]=NORMALIZE_Z(z2[j]);
    }
  }

  {
    meshdata *meshi;
    meshi=meshinfo;
    veclength = meshi->xplt[1]-meshi->xplt[0];
  }
  min_gridcell_size=meshinfo->xplt[1]-meshinfo->xplt[0];
  for(i=0;i<nmeshes;i++){
    float dx, dy, dz;
    meshdata *meshi;

    meshi=meshinfo+i;
    dx = meshi->xplt[1] - meshi->xplt[0];
    dy = meshi->yplt[1] - meshi->yplt[0];
    dz = meshi->zplt[1] - meshi->zplt[0];
    min_gridcell_size=MIN(dx,min_gridcell_size);
    min_gridcell_size=MIN(dy,min_gridcell_size);
    min_gridcell_size=MIN(dz,min_gridcell_size);
  }
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;

    meshi=meshinfo+i;
    if(veclength>meshi->xplt[1]-meshi->xplt[0])veclength=meshi->xplt[1]-meshi->xplt[0];
    if(veclength>meshi->yplt[1]-meshi->yplt[0])veclength=meshi->yplt[1]-meshi->yplt[0];
    if(veclength>meshi->zplt[1]-meshi->zplt[0])veclength=meshi->zplt[1]-meshi->zplt[0];
  }
  veclength = SCALE2SMV(veclength);
  veclength = 0.01;

  for(igrid=0;igrid<nmeshes;igrid++){
    meshdata *meshi;
    float *face_centers;
    float *xplt_cen, *yplt_cen, *zplt_cen;
    int ibar, jbar, kbar;
    float *xplt_orig, *yplt_orig, *zplt_orig;
    float *xplt, *yplt, *zplt;
    int j,k;
    float dx, dy, dz;
    float *dplane_min, *dplane_max;

    meshi=meshinfo+igrid;
    ibar=meshi->ibar;
    jbar=meshi->jbar;
    kbar=meshi->kbar;
    xplt_orig = meshi->xplt_orig;
    yplt_orig = meshi->yplt_orig;
    zplt_orig = meshi->zplt_orig;
    xplt = meshi->xplt;
    yplt = meshi->yplt;
    zplt = meshi->zplt;
    xplt_cen = meshi->xplt_cen;
    yplt_cen = meshi->yplt_cen;
    zplt_cen = meshi->zplt_cen;

    for(i=0;i<ibar+1;i++){
      xplt_orig[i]=xplt[i];
      xplt[i]=NORMALIZE_X(xplt[i]);
    }
    for(j=0;j<jbar+1;j++){
      yplt_orig[j]=yplt[j];
      yplt[j]=NORMALIZE_Y(yplt[j]);
    }
    for(k=0;k<kbar+1;k++){
      zplt_orig[k]=zplt[k];
      zplt[k]=NORMALIZE_Z(zplt[k]);
    }

    for(nn=0;nn<ibar;nn++){
      xplt_cen[nn]=(xplt[nn]+xplt[nn+1])/2.0;
    }
    for(nn=0;nn<jbar;nn++){
      yplt_cen[nn]=(yplt[nn]+yplt[nn+1])/2.0;
    }
    for(nn=0;nn<kbar;nn++){
      zplt_cen[nn]=(zplt[nn]+zplt[nn+1])/2.0;
    }

    meshi->boxoffset=-(zplt[1]-zplt[0])/10.0;
    meshi->boxmin[0]=xplt_orig[0];
    meshi->boxmin[1]=yplt_orig[0];
    meshi->boxmin[2]=zplt_orig[0];
    meshi->boxmax[0]=xplt_orig[ibar];
    meshi->boxmax[1]=yplt_orig[jbar];
    meshi->boxmax[2]=zplt_orig[kbar];
    meshi->dbox[0]=meshi->boxmax[0]-meshi->boxmin[0];
    meshi->dbox[1]=meshi->boxmax[1]-meshi->boxmin[1];
    meshi->dbox[2]=meshi->boxmax[2]-meshi->boxmin[2];
    meshi->boxmiddle[0] = meshi->boxmin[0]+meshi->dbox[0]/2.0;
    meshi->boxmiddle[1] = meshi->boxmin[1]+meshi->dbox[1]/2.0;
    meshi->boxmiddle[2] = meshi->boxmin[2]+meshi->dbox[2]/2.0;
    meshi->boxeps[0]=0.5*meshi->dbox[0]/(float)ibar;
    meshi->boxeps[1]=0.5*meshi->dbox[1]/(float)jbar;
    meshi->boxeps[2]=0.5*meshi->dbox[2]/(float)kbar;
    meshi->dcell3[0] = xplt[1]-xplt[0];
    meshi->dcell3[1] = yplt[1]-yplt[0];
    meshi->dcell3[2] = zplt[1]-zplt[0];
    NORMALIZE_XYZ(meshi->boxmin_scaled,meshi->boxmin);
    NORMALIZE_XYZ(meshi->boxmax_scaled,meshi->boxmax);
    NORMALIZE_XYZ(meshi->boxmiddle_scaled, meshi->boxmiddle);
    meshi->x0 = xplt[0];
    meshi->x1 = xplt[ibar];
    meshi->y0 = yplt[0];
    meshi->y1 = yplt[jbar];
    meshi->z0 = zplt[0];
    meshi->z1 = zplt[kbar];
    dx = xplt[1]-xplt[0];
    dy = yplt[1]-yplt[0];
    dz = zplt[1]-zplt[0];
    meshi->dcell = sqrt(dx*dx+dy*dy+dz*dz);

    dplane_min = meshi->dplane_min;
    dplane_min[0] = MIN(MIN(dx, dy), dz);
    dplane_min[1] = MIN(dy, dz);
    dplane_min[2] = MIN(dx, dz);
    dplane_min[3] = MIN(dx, dy);

    dplane_max = meshi->dplane_max;
    dplane_max[0] = MAX(MAX(dx, dy), dz);
    dplane_max[1] = MAX(dy, dz);
    dplane_max[2] = MAX(dx, dz);
    dplane_max[3] = MAX(dx, dy);

    face_centers = meshi->face_centers;
    for(j=0;j<6;j++){
      face_centers[0]=meshi->xcen;
      face_centers[1]=meshi->ycen;
      face_centers[2]=meshi->zcen;
      face_centers+=3;
    }
    face_centers = meshi->face_centers;
    face_centers[0]=meshi->boxmin_scaled[0];
    face_centers[3]=meshi->boxmax_scaled[0];
    face_centers[7]=meshi->boxmin_scaled[1];
    face_centers[10]=meshi->boxmax_scaled[1];
    face_centers[14]=meshi->boxmin_scaled[2];
    face_centers[17]=meshi->boxmax_scaled[2];
  }

  ntransparentblocks=0;
  ntransparentvents=0;
  nopenvents=0;
  nopenvents_nonoutline=0;
  ndummyvents=0;
  for(igrid=0;igrid<nmeshes;igrid++){
    meshdata *meshi;

    meshi=meshinfo+igrid;
    for(i=0;i<meshi->nbptrs;i++){
      blockagedata *bc;

      bc=meshi->blockageinfoptrs[i];
      if(bc->color[3]<0.99)ntransparentblocks++;
    }
    for(i=0;i<meshi->nvents;i++){
      ventdata *vi;

      vi = meshi->ventinfo + i;
      if(vi->isOpenvent==1){
        nopenvents++;
        if(vi->type!=BLOCK_OUTLINE)nopenvents_nonoutline++;
      }
      if(vi->dummy==1)ndummyvents++;
      if(vi->color[3]<0.99)ntransparentvents++;
    }
  }

  for(igrid=0;igrid<nmeshes;igrid++){
    meshdata *meshi;

    meshi=meshinfo+igrid;
    for(i=0;i<meshi->nbptrs;i++){
      blockagedata *bc;

      bc=meshi->blockageinfoptrs[i];
      bc->xmin += meshi->offset[XXX];
      bc->xmax += meshi->offset[XXX];
      bc->ymin += meshi->offset[YYY];
      bc->ymax += meshi->offset[YYY];
      bc->zmin += meshi->offset[ZZZ];
      bc->zmax += meshi->offset[ZZZ];
      bc->xmin = NORMALIZE_X(bc->xmin);
      bc->xmax = NORMALIZE_X(bc->xmax);
      bc->ymin = NORMALIZE_Y(bc->ymin);
      bc->ymax = NORMALIZE_Y(bc->ymax);
      bc->zmin = NORMALIZE_Z(bc->zmin);
      bc->zmax = NORMALIZE_Z(bc->zmax);
      bc->xyzORIG[0]=bc->xmin;
      bc->xyzORIG[1]=bc->xmax;
      bc->xyzORIG[2]=bc->ymin;
      bc->xyzORIG[3]=bc->ymax;
      bc->xyzORIG[4]=bc->zmin;
      bc->xyzORIG[5]=bc->zmax;
      bc->ijkORIG[0]=bc->ijk[0];
      bc->ijkORIG[1]=bc->ijk[1];
      bc->ijkORIG[2]=bc->ijk[2];
      bc->ijkORIG[3]=bc->ijk[3];
      bc->ijkORIG[4]=bc->ijk[4];
      bc->ijkORIG[5]=bc->ijk[5];
    }
    for(i=0;i<meshi->nvents+12;i++){
      ventdata *vi;

      vi=meshi->ventinfo+i;
      vi->xmin = NORMALIZE_X(vi->xmin);
      vi->xmax = NORMALIZE_X(vi->xmax);
      vi->ymin = NORMALIZE_Y(vi->ymin);
      vi->ymax = NORMALIZE_Y(vi->ymax);
      vi->zmin = NORMALIZE_Z(vi->zmin);
      vi->zmax = NORMALIZE_Z(vi->zmax);
    }
  }
  for(igrid=0;igrid<nmeshes;igrid++){
    meshdata *meshi;

    meshi=meshinfo+igrid;
    for(i=0;i<meshi->nbptrs;i++){
      blockagedata *bc;

      bc=meshi->blockageinfoptrs[i];
      BackupBlockage(bc);
    }
  }

  for(i=0;i<ncadgeom;i++){
    cadgeomdata *cd;
    int j;

    cd=cadgeominfo+i;
    for(j=0;j<cd->nquads;j++){
      int k;
      cadquad *quadi;

      quadi = cd->quad+j;
      for(k=0;k<4;k++){
        NORMALIZE_XYZ(quadi->xyzpoints+3*k,quadi->xyzpoints+3*k);
      }
      if(cd->version==2&&quadi->cadlookq->textureinfo.loaded==1){
        UpdateCADTextCoords(quadi);
      }
    }
  }
  for(n=0;n<nrooms;n++){
    roomdata *roomi;

    roomi = roominfo + n;
    roomi->x0=NORMALIZE_X(roomi->x0);
    roomi->y0=NORMALIZE_Y(roomi->y0);
    roomi->z0=NORMALIZE_Z(roomi->z0);
    roomi->x1=NORMALIZE_X(roomi->x1);
    roomi->y1=NORMALIZE_Y(roomi->y1);
    roomi->z1=NORMALIZE_Z(roomi->z1);
    roomi->dx=SCALE2SMV(roomi->dx);
    roomi->dy=SCALE2SMV(roomi->dy);
    roomi->dz=SCALE2SMV(roomi->dz);
  }
  for(n=0;n<nfires;n++){
    firedata *firen;

    firen = fireinfo + n;
    firen->absx=NORMALIZE_X(firen->absx);
    firen->absy=NORMALIZE_Y(firen->absy);
    firen->absz=NORMALIZE_Z(firen->absz);
    firen->dz=SCALE2SMV(firen->dz);
  }
  for(n=0;n<nzvents;n++){
    zventdata *zvi;

    zvi = zventinfo + n;

    zvi->x0 = NORMALIZE_X(zvi->x0);
    zvi->x1 = NORMALIZE_X(zvi->x1);
    zvi->y0 = NORMALIZE_Y(zvi->y0);
    zvi->y1 = NORMALIZE_Y(zvi->y1);
    zvi->z0 = NORMALIZE_Z(zvi->z0);
    zvi->z1 = NORMALIZE_Z(zvi->z1);
  }

  for(i=0;i<nmeshes;i++){
    meshdata *meshi;
    float *xspr, *yspr, *zspr;
    float *xsprplot, *ysprplot, *zsprplot;
    float *xheat, *yheat, *zheat;
    float *xheatplot, *yheatplot, *zheatplot;
    float *offset;

    meshi=meshinfo + i;
    offset = meshi->offset;
    xsprplot = meshi->xsprplot;
    ysprplot = meshi->ysprplot;
    zsprplot = meshi->zsprplot;
    xspr = meshi->xspr;
    yspr = meshi->yspr;
    zspr = meshi->zspr;
    xheatplot = meshi->xheatplot;
    yheatplot = meshi->yheatplot;
    zheatplot = meshi->zheatplot;
    xheat = meshi->xheat;
    yheat = meshi->yheat;
    zheat = meshi->zheat;
    for(n=0;n<meshi->nspr;n++){
      xsprplot[n]=NORMALIZE_X(offset[XXX]+xspr[n]);
      ysprplot[n]=NORMALIZE_Y(offset[YYY]+yspr[n]);
      zsprplot[n]=NORMALIZE_Z(offset[ZZZ]+zspr[n]);
    }
    for(n=0;n<meshi->nheat;n++){
      xheatplot[n]=NORMALIZE_X(offset[XXX]+xheat[n]);
      yheatplot[n]=NORMALIZE_Y(offset[YYY]+yheat[n]);
      zheatplot[n]=NORMALIZE_Z(offset[ZZZ]+zheat[n]);
    }
    for(n=0;n<meshi->nvents+12;n++){
      ventdata *vi;

      vi = meshi->ventinfo+n;
      vi->xvent1plot=NORMALIZE_X(offset[XXX]+vi->xvent1);
      vi->xvent2plot=NORMALIZE_X(offset[XXX]+vi->xvent2);
      vi->yvent1plot=NORMALIZE_Y(offset[YYY]+vi->yvent1);
      vi->yvent2plot=NORMALIZE_Y(offset[YYY]+vi->yvent2);
      vi->zvent1plot=NORMALIZE_Z(offset[ZZZ]+vi->zvent1);
      vi->zvent2plot=NORMALIZE_Z(offset[ZZZ]+vi->zvent2);
    }
  }
  UpdateVentOffset();
  if(nsmoke3dinfo>0)NewMemory((void **)&smoke3dinfo_sorted,nsmoke3dinfo*sizeof(smoke3ddata *));
  NewMemory((void **)&meshvisptr,nmeshes*sizeof(int));
  for(i=0;i<nmeshes;i++){
    meshvisptr[i]=1;
  }
  for(i=0;i<nsmoke3dinfo;i++){
    smoke3dinfo_sorted[i]=smoke3dinfo+i;
  }
}

/* ------------------ IsSliceDup ------------------------ */

int IsSliceDup(slicedata *sd, int nslice){
  int i;

  for(i=0;i<nslice-1;i++){
    slicedata *slicei;

    slicei = sliceinfo + i;
    if(slicei->ijk_min[0]!=sd->ijk_min[0]||slicei->ijk_max[0]!=sd->ijk_max[0])continue;
    if(slicei->ijk_min[1]!=sd->ijk_min[1]||slicei->ijk_max[1]!=sd->ijk_max[1])continue;
    if(slicei->ijk_min[2]!=sd->ijk_min[2]||slicei->ijk_max[2]!=sd->ijk_max[2])continue;
    if(strcmp(slicei->label.longlabel,sd->label.longlabel)!=0)continue;
    if(slicei->slicetype!=sd->slicetype)continue;
    if(slicei->blocknumber!=sd->blocknumber)continue;
    if(slicei->volslice!=sd->volslice)continue;
    if(slicei->idir!=sd->idir)continue;
    return 1;
  }
  return 0;
}

/* ------------------ CreateNullLabel ------------------------ */

int CreateNullLabel(flowlabels *flowlabel){
  char buffer[255];
  size_t len;

  len = strlen("Particles");
  strcpy(buffer, "Particles");
  TrimBack(buffer);
  len = strlen(buffer);
  if(NewMemory((void **)&flowlabel->longlabel, (unsigned int)(len + 1)) == 0)return 2;
  STRCPY(flowlabel->longlabel, buffer);


  len = strlen("Particles");
  strcpy(buffer, "Particles");
  len = strlen(buffer);
  TrimBack(buffer);
  len = strlen(buffer);
  if(NewMemory((void **)&flowlabel->shortlabel, (unsigned int)(len + 1)) == 0)return 2;
  STRCPY(flowlabel->shortlabel, buffer);

  len = strlen("Particles");
  strcpy(buffer, "Particles");
  len = strlen(buffer);
  TrimBack(buffer);
  len = strlen(buffer);
  if(NewMemory((void *)&flowlabel->unit, (unsigned int)(len + 1)) == 0)return 2;
  STRCPY(flowlabel->unit, buffer);
  return 0;
}

/* ------------------ GetSurface ------------------------ */

surfdata *GetSurface(char *label){
  int i;

  for(i = 0; i < nsurfinfo; i++){
    surfdata *surfi;

    surfi = surfinfo + i;
    if(strcmp(surfi->surfacelabel, label) == 0)return surfi;
  }
  return surfacedefault;
}

/* ------------------ InitEvacProp ------------------------ */

void InitEvacProp(void){
  char label[256];
  char *smokeview_id;
  int nsmokeview_ids;

  strcpy(label, "evac default");

  NewMemory((void **)&prop_evacdefault, sizeof(propdata));
  nsmokeview_ids = 1;

  InitProp(prop_evacdefault, nsmokeview_ids, label);


  NewMemory((void **)&smokeview_id, 6 + 1);
  strcpy(smokeview_id, "sensor");
  prop_evacdefault->smokeview_ids[0] = smokeview_id;
  prop_evacdefault->smv_objects[0] = GetSmvObjectType(prop_evacdefault->smokeview_ids[0], missing_device);

  prop_evacdefault->smv_object = prop_evacdefault->smv_objects[0];
  prop_evacdefault->smokeview_id = prop_evacdefault->smokeview_ids[0];

  prop_evacdefault->ntextures = 0;
}

/* ------------------ InitMatl ------------------------ */

void InitMatl(matldata *matl){
  matl->matllabel = NULL;
  matl->color = block_ambient2;
}

/* ------------------ InitObst ------------------------ */

void InitObst(blockagedata *bc, surfdata *surf, int index, int meshindex){
  int colorindex, blocktype;
  int i;
  char blocklabel[255];
  size_t len;

  bc->prop = NULL;
  bc->is_wuiblock = 0;
  bc->transparent = 0;
  bc->usecolorindex = 0;
  bc->colorindex = -1;
  bc->nshowtime = 0;
  bc->hole = 0;
  bc->showtime = NULL;
  bc->showhide = NULL;
  bc->showtimelist = NULL;
  bc->show = 1;
  bc->blockage_id = index;
  bc->meshindex = meshindex;
  bc->hidden = 0;
  bc->invisible = 0;
  bc->texture_origin[0] = texture_origin[0];
  bc->texture_origin[1] = texture_origin[1];
  bc->texture_origin[2] = texture_origin[2];

  /*
  new OBST format:
  i1 i2 j1 j2 k1 k2 colorindex blocktype       : if blocktype!=1&&colorindex!=-3
  i1 i2 j1 j2 k1 k2 colorindex blocktype r g b : if blocktype!=1&&colorindex==-3
  i1 i2 j1 j2 k1 k2 ceiling texture blocktype [wall texture floor texture] : if blocktype==1
  int colorindex, blocktype;
  colorindex: -1 default color
  -2 invisible
  -3 use r g b color
  >=0 color/color2/texture index
  blocktype: 0 regular block non-textured
  1 regular block textured
  2 outline
  3 smoothed block
  r g b           colors used if colorindex==-3
  */
  colorindex = -1;
  blocktype = 0;
  bc->ijk[IMIN] = 0;
  bc->ijk[IMAX] = 1;
  bc->ijk[JMIN] = 0;
  bc->ijk[JMAX] = 1;
  bc->ijk[KMIN] = 0;
  bc->ijk[KMAX] = 1;
  bc->colorindex = colorindex;
  bc->type = blocktype;

  bc->del = 0;
  bc->changed = 0;
  bc->changed_surface = 0;
  bc->walltype = WALL_1;

  bc->color = surf->color;
  bc->useblockcolor = 0;
  for(i = 0; i<6; i++){
    bc->surf_index[i] = -1;
    bc->surf[i] = surf;
    bc->faceinfo[i] = NULL;
  }
  for(i = 0; i<7; i++){
    bc->patchvis[i] = 1;
  }
  sprintf(blocklabel, "**blockage %i", index);
  len = strlen(blocklabel);
  NewMemory((void **)&bc->label, ((unsigned int)(len + 1))*sizeof(char));
  strcpy(bc->label, blocklabel);
}

/* ------------------ InitSurface ------------------------ */

void InitSurface(surfdata *surf){
  surf->iso_level = -1;
  surf->used_by_obst = 0;
  surf->used_by_vent = 0;
  surf->emis = 1.0;
  surf->temp_ignition = TEMP_IGNITION_MAX;
  surf->surfacelabel = NULL;
  surf->texturefile = NULL;
  surf->textureinfo = NULL;
  surf->color = block_ambient2;
  surf->t_width = 1.0;
  surf->t_height = 1.0;
  surf->type = BLOCK_regular;
  surf->obst_surface = 1;
  surf->location = 0;
  surf->invisible = 0;
  surf->transparent = 0;
}

/* ------------------ InitVentSurface ------------------------ */

void InitVentSurface(surfdata *surf){
  surf->emis = 1.0;
  surf->temp_ignition = TEMP_IGNITION_MAX;
  surf->surfacelabel = NULL;
  surf->texturefile = NULL;
  surf->textureinfo = NULL;
  surf->color = ventcolor;
  surf->t_width = 1.0;
  surf->t_height = 1.0;
  surf->type = BLOCK_outline;
  surf->obst_surface = 0;

}

/* ----------------------- ReadDeviceHeader ----------------------------- */
#define BUFFER_LEN 255

void ReadDeviceHeader(char *file, devicedata *devices, int ndevices){
  FILE *stream;
  devicedata *devicecopy;
  char buffer[BUFFER_LEN], *comma;
  int buffer_len = BUFFER_LEN;

  if(file == NULL)return;
  stream = fopen(file, "r");
  if(stream == NULL)return;

  devicecopy = devices;

  while(!feof(stream)){
    fgets(buffer, buffer_len, stream);
    comma = strchr(buffer, ',');
    if(comma != NULL)*comma = 0;
    TrimBack(buffer);
    if(strcmp(buffer, "//DATA") == 0){
      break;
    }
    if(strcmp(buffer, "DEVICE") == 0){
      ParseDevicekeyword2(stream, devicecopy);
      devicecopy++;
    }
  }
  fclose(stream);
}

/* ------------------ SetSurfaceIndex ------------------------ */

void SetSurfaceIndex(blockagedata *bc){
  int i, j, jj;
  surfdata *surfj;
  char *surflabel, *bclabel;
  int *surf_index;
  int wall_flag;

  for(i = 0; i < 6; i++){
    bc->surf_index[i] = -1;
    bclabel = bc->surf[i]->surfacelabel;
    if(bc->surf[i] == NULL)continue;
    for(jj = 1; jj < nsurfinfo + 1; jj++){
      j = jj;
      if(jj == nsurfinfo)j = 0;
      surfj = surfinfo + j;
      surflabel = surfj->surfacelabel;
      if(strcmp(bclabel, surflabel) != 0)continue;
      bc->surf_index[i] = j;
      break;
    }
  }
  surf_index = bc->surf_index;
  wall_flag = 1;
  for(i = 1; i < 6; i++){
    if(surf_index[i] != surf_index[0]){
      wall_flag = 0;
      break;
    }
  }
  if(wall_flag == 1){
    bc->walltype = WALL_1;
    bc->walltypeORIG = WALL_1;
    return;
  }
  if(
    surf_index[UP_X] == surf_index[DOWN_X] &&
    surf_index[DOWN_Y] == surf_index[DOWN_X] &&
    surf_index[UP_Y] == surf_index[DOWN_X]
    ){
    bc->walltype = WALL_3;
    bc->walltypeORIG = WALL_3;
    return;
  }
  bc->walltype = WALL_6;
  bc->walltypeORIG = WALL_6;

}

/* ------------------ SurfIdCompare ------------------------ */

int SurfIdCompare(const void *arg1, const void *arg2){
  surfdata *surfi, *surfj;
  int i, j;

  i = *(int *)arg1;
  j = *(int *)arg2;

  surfi = surfinfo+i;
  surfj = surfinfo+j;

  return(strcmp(surfi->surfacelabel, surfj->surfacelabel));
}

/* ------------------ updated_sorted_surfidlist ------------------------ */

void UpdateSortedSurfIdList(void){
  int i;

  FREEMEMORY(sorted_surfidlist);
  FREEMEMORY(inv_sorted_surfidlist);
  NewMemory((void **)&sorted_surfidlist, nsurfinfo*sizeof(int));
  NewMemory((void **)&inv_sorted_surfidlist, nsurfinfo*sizeof(int));


  nsorted_surfidlist = nsurfinfo;
  for(i = 0; i<nsorted_surfidlist; i++){
    sorted_surfidlist[i] = i;
  }

  qsort((int *)sorted_surfidlist, (size_t)nsurfinfo, sizeof(int), SurfIdCompare);
  for(i = 0; i<nsorted_surfidlist; i++){
    inv_sorted_surfidlist[sorted_surfidlist[i]] = i;
  }
}

/* ------------------ ParseDatabase ------------------------ */

void ParseDatabase(char *file){
  FILE *stream;
  char buffer[1000], *buffer2 = NULL, *buffer3, *slashptr;
  size_t lenbuffer, lenbuffer2;
  size_t sizebuffer2;
  char *surf_id = NULL, *start, *surf_id2;
  char *c;
  int i, j;
  surfdata *surfj;
  char *labeli, *labelj;
  int nexti;
  int nsurfids_shown;

  /* free memory called before */

  for(i = 0; i<nsurfids; i++){
    surf_id = surfids[i].label;
    FREEMEMORY(surf_id);
  }
  FREEMEMORY(surfids);
  nsurfids = 0;


  if(file==NULL||strlen(file)==0||(stream = fopen(file, "r"))==NULL){
    NewMemory((void **)&surfids, (nsurfids+1)*sizeof(surfid));
    surf_id = NULL;
    NewMemory((void **)&surf_id, 6);
    strcpy(surf_id, "INERT");
    surfids[0].label = surf_id;
    surfids[0].location = 0;
    surfids[0].show = 1;
    nsurfids = 1;
  }

  else{
    sizebuffer2 = 1001;
    NewMemory((void **)&buffer2, sizebuffer2);

    /* find out how many surfs are in database file so memory can be allocated */

    while(!feof(stream)){
      if(fgets(buffer, 1000, stream)==NULL)break;
      if(STRSTR(buffer, "&SURF")==NULL)continue;


      slashptr = strstr(buffer, "/");
      if(slashptr!=NULL)strcpy(buffer2, buffer);
      buffer3 = buffer;
      while(slashptr!=NULL){
        fgets(buffer, 1000, stream);
        lenbuffer = strlen(buffer);
        lenbuffer2 = strlen(buffer2);
        if(lenbuffer2+lenbuffer+2>sizebuffer2){
          sizebuffer2 = lenbuffer2+lenbuffer+2+1000;
          ResizeMemory((void **)&buffer2, (unsigned int)sizebuffer2);
        }
        strcat(buffer2, buffer);
        slashptr = strstr(buffer, "/");
        buffer3 = buffer2;
      }
      start = STRSTR(buffer3, "ID");
      if(start!=NULL)nsurfids++;
    }

    /* allocate memory */

    NewMemory((void **)&surfids, (nsurfids+1)*sizeof(surfid));
    surf_id = NULL;
    NewMemory((void **)&surf_id, 6);
    strcpy(surf_id, "INERT");
    surfids[0].label = surf_id;
    surfids[0].location = 0;
    surfids[0].show = 1;


    /* now look for IDs and copy them into an array */

    rewind(stream);
    nsurfids = 1;
    while(!feof(stream)){
      if(fgets(buffer, 1000, stream)==NULL)break;
      if(STRSTR(buffer, "&SURF")==NULL)continue;


      slashptr = strstr(buffer, "/");
      if(slashptr!=NULL)strcpy(buffer2, buffer);
      buffer3 = buffer2;
      while(slashptr!=NULL){
        fgets(buffer, 1000, stream);
        lenbuffer = strlen(buffer);
        lenbuffer2 = strlen(buffer2);
        if(lenbuffer2+lenbuffer+2>sizebuffer2){
          sizebuffer2 = lenbuffer2+lenbuffer+2+1000;
          ResizeMemory((void **)&buffer2, (unsigned int)sizebuffer2);
        }
        strcat(buffer2, buffer);
        slashptr = strstr(buffer, "/");
        buffer3 = buffer2;
      }
      start = STRSTR(buffer3+3, "ID");
      if(start!=NULL)nsurfids++;
      surf_id = NULL;
      surf_id2 = NULL;
      for(c = start; c!=NULL&&*c!='\0'; c++){
        if(surf_id==NULL&&*c=='\''){
          surf_id = c+1;
          continue;
        }
        if(surf_id!=NULL&&*c=='\''){
          *c = '\0';
          NewMemory((void **)&surf_id2, strlen(surf_id)+1);
          strcpy(surf_id2, surf_id);
          surfids[nsurfids-1].label = surf_id2;
          surfids[nsurfids-1].location = 1;
          surfids[nsurfids-1].show = 1;
          break;
        }
      }

    }
  }

  /* identify duplicate surfaces */
  /*** debug: make sure ->show is defined for all cases ***/

  nsurfids_shown = 0;
  for(i = 0; i<nsurfids; i++){
    labeli = surfids[i].label;
    nexti = 0;
    for(j = 0; j<nsurfinfo; j++){
      surfj = surfinfo+j;
      labelj = surfj->surfacelabel;
      if(strcmp(labeli, labelj)==0){
        nexti = 1;
        break;
      }
    }
    if(nexti==1){
      surfids[i].show = 0;
      continue;
    }
    for(j = 0; j<i; j++){
      labelj = surfids[j].label;
      if(strcmp(labeli, labelj)==0){
        nexti = 1;
        break;
      }
    }
    if(nexti==1){
      surfids[i].show = 0;
      continue;
    }
    nsurfids_shown++;

  }

  /* add surfaces found in database to those surfaces defined in previous SURF lines */

  if(nsurfids_shown>0){
    if(nsurfinfo==0){
      FREEMEMORY(surfinfo);
      FREEMEMORY(textureinfo);
      NewMemory((void **)&surfinfo, (nsurfids_shown+MAX_ISO_COLORS+1)*sizeof(surfdata));
      NewMemory((void **)&textureinfo, nsurfids_shown*sizeof(surfdata));
    }
    if(nsurfinfo>0){
      if(surfinfo==NULL){
        NewMemory((void **)&surfinfo, (nsurfids_shown+nsurfinfo+MAX_ISO_COLORS+1)*sizeof(surfdata));
      }
      else{
        ResizeMemory((void **)&surfinfo, (nsurfids_shown+nsurfinfo+MAX_ISO_COLORS+1)*sizeof(surfdata));
      }
      if(textureinfo==NULL){
        NewMemory((void **)&textureinfo, (nsurfids_shown+nsurfinfo)*sizeof(surfdata));
      }
      else{
        ResizeMemory((void **)&textureinfo, (nsurfids_shown+nsurfinfo)*sizeof(surfdata));
      }
    }
    surfj = surfinfo+nsurfinfo-1;
    for(j = 0; j<nsurfids; j++){
      if(surfids[j].show==0)continue;
      surfj++;
      InitSurface(surfj);
      surfj->surfacelabel = surfids[j].label;
    }
    nsurfinfo += nsurfids_shown;
  }
  UpdateSortedSurfIdList();
}

/* ------------------ ReadZVentData ------------------------ */

void ReadZVentData(zventdata *zvi, char *buffer, int flag){
  float dxyz[3];
  float xyz[6];
  float color[4];
  roomdata *roomi;
  int roomfrom=-1, roomto=-1;
  int vertical_vent_type=0;
  float area_fraction = 1.0;

  color[0]=1.0;
  color[1]=0.0;
  color[2]=1.0;
  color[3]=1.0;
  if(flag==ZVENT_2ROOM){
    sscanf(buffer, "%i %i %f %f %f %f %f %f %i",
      &roomfrom, &roomto, xyz, xyz + 1, xyz + 2, xyz + 3, xyz + 4, xyz + 5,
      &vertical_vent_type
  );
  }
  else{
    sscanf(buffer, "%i %f %f %f %f %f %f",
      &roomfrom, xyz, xyz + 1, xyz + 2, xyz + 3, xyz + 4, xyz + 5);
    roomto = roomfrom;
  }

  if(roomfrom<1 || roomfrom>nrooms)roomfrom = nrooms + 1;
  roomi = roominfo + roomfrom - 1;
  zvi->room1 = roomi;
  if(roomto<1 || roomto>nrooms)roomto = nrooms + 1;
  zvi->room2 = roominfo + roomto - 1;
  zvi->x0 = roomi->x0 + xyz[0];
  zvi->x1 = roomi->x0 + xyz[1];
  zvi->y0 = roomi->y0 + xyz[2];
  zvi->y1 = roomi->y0 + xyz[3];
  zvi->z0 = roomi->z0 + xyz[4];
  zvi->z1 = roomi->z0 + xyz[5];
  zvi->xcen = (zvi->x0 + zvi->x1)/2.0;
  zvi->ycen = (zvi->y0 + zvi->y1)/2.0;
  dxyz[0] = ABS(xyz[0] - xyz[1]);
  dxyz[1] = ABS(xyz[2] - xyz[3]);
  dxyz[2] = ABS(xyz[4] - xyz[5]);
  zvi->area = 1.0;
  zvi->vertical_vent_type = vertical_vent_type;
  if(dxyz[0] > 0.0)zvi->area *= dxyz[0];
  if(dxyz[1] > 0.0)zvi->area *= dxyz[1];
  if(dxyz[2] > 0.0)zvi->area *= dxyz[2];
  zvi->radius = sqrt(zvi->area/PI);

  // see which side of room vent is closest too
  if(dxyz[0] < MIN(dxyz[1], dxyz[2])){
    if(ABS(zvi->x0 - roomi->x0) < ABS(zvi->x0 - roomi->x1)){
      zvi->wall = LEFT_WALL;
    }
    else{
      zvi->wall = RIGHT_WALL;
    }
  }
  else if(dxyz[1] < MIN(dxyz[0], dxyz[2])){
    if(ABS(zvi->y0 - roomi->y0) < ABS(zvi->y0 - roomi->y1)){
      zvi->wall = FRONT_WALL;
    }
    else{
      zvi->wall = BACK_WALL;
    }
  }
  else{
    zvi->wall = BOTTOM_WALL;
    if(ABS(zvi->z0 - roomi->z0) < ABS(zvi->z0 - roomi->z1)){
      zvi->wall = BOTTOM_WALL;
    }
    else{
      zvi->wall = TOP_WALL;
    }
  }
  zvi->color = GetColorPtr(color);
  zvi->area_fraction = area_fraction;
}

/* ------------------ SetupMeshWalls ------------------------ */

void SetupMeshWalls(void){
  int i;
  int ncount = 0;

  for(i = 0; i < nmeshes; i++){
    meshdata *meshi;
    float xyz[3], *bmin, *bmax, bmid[3];
    int *is_extface;

    meshi = meshinfo + i;
    bmin = meshi->boxmin;
    bmax = meshi->boxmax;
    is_extface = meshi->is_extface;

    bmid[0] = (bmin[0] + bmax[0]) / 2.0;
    bmid[1] = (bmin[1] + bmax[1]) / 2.0;
    bmid[2] = (bmin[2] + bmax[2]) / 2.0;

#define EPSMESH 0.001
    xyz[0] = bmin[0] - EPSMESH;
    xyz[1] = bmid[1];
    xyz[2] = bmid[2];
    if(GetMesh(xyz) != NULL){
      is_extface[0] = 0;
      ncount++;
    }

    xyz[0] = bmax[0] + EPSMESH;
    xyz[1] = bmid[1];
    xyz[2] = bmid[2];
    if(GetMesh(xyz) != NULL){
      is_extface[1] = 0;
      ncount++;
    }

    xyz[0] = bmid[0];
    xyz[1] = bmin[1] - EPSMESH;
    xyz[2] = bmid[2];
    if(GetMesh(xyz) != NULL){
      is_extface[2] = 0;
      ncount++;
    }

    xyz[0] = bmid[0];
    xyz[1] = bmax[1] + EPSMESH;
    xyz[2] = bmid[2];
    if(GetMesh(xyz) != NULL){
      is_extface[3] = 0;
      ncount++;
    }

    xyz[0] = bmid[0];
    xyz[1] = bmid[1];
    xyz[2] = bmin[2] - EPSMESH;
    if(GetMesh(xyz) != NULL){
      is_extface[4] = 0;
      ncount++;
    }

    xyz[0] = bmid[0];
    xyz[1] = bmid[1];
    xyz[2] = bmax[2] + EPSMESH;
    if(GetMesh(xyz) != NULL){
      is_extface[5] = 0;
      ncount++;
    }
  }
}

/* ------------------ MakeFileLists ------------------------ */

void MakeFileLists(void){
  char filter_casedir[256], filter_casename[256];

  // create list of all files for the case being visualized (casename*.* )

  strcpy(filter_casename, "");
  if(fdsprefix != NULL&&strlen(fdsprefix) > 0){
    strcat(filter_casename, fdsprefix);
    strcat(filter_casename, "*");
  }

  // create a list of all files in the current directory

  nfilelist_casename = GetFileListSize(".", filter_casename);
  MakeFileList(".", filter_casename, nfilelist_casename, YES, &filelist_casename);

  strcpy(filter_casedir, "");
  nfilelist_casedir = GetFileListSize(".", filter_casedir);
  MakeFileList(".", filter_casedir, nfilelist_casedir, YES, &filelist_casedir);
}

/* ------------------ ReadSMV ------------------------ */

int ReadSMV(char *file, char *file2){

/* read the .smv file */
  float read_time, processing_time, wrapup_time, getfilelist_time;
  float pass0_time, pass1_time, pass2_time, pass3_time, pass4_time, pass5_time;
  int have_zonevents,nzventsnew=0;
  int unit_start=20;
  devicedata *devicecopy;
  int do_pass4=0, do_pass5=0;
  int roomdefined=0;
  int errorcode;
  int noGRIDpresent=1,startpass;
  slicedata *sliceinfo_copy=NULL;
  int nisos_per_mesh=1;

  int nn_smoke3d=0,nn_patch=0,nn_iso=0,nn_part=0,nn_slice=0,nslicefiles=0,nvents;

  int ipart=0, islicecount=1, ipatch=0, iroom=0,izone_local=0,ifire=0,iiso=0;
  int ismoke3d=0,ismoke3dcount=1,igrid,ioffset;
  int itrnx, itrny, itrnz, ipdim, iobst, ivent, icvent;
  int ibartemp=2, jbartemp=2, kbartemp=2;

  int setGRID=0;
  int  i;

  char buffer[256],buffer2[256],*bufferptr;
#ifdef pp_READBUFFER
  bufferstreamdata streaminfo, *stream=&streaminfo;
#else
  FILE *stream=NULL,*stream1=NULL,*stream2=NULL;
#endif

  START_TIMER(processing_time);

  START_TIMER(getfilelist_time);
  MakeFileLists();
  STOP_TIMER(getfilelist_time);

  START_TIMER(pass0_time);
  START_TIMER(read_time);

#ifdef pp_READBUFFER
  if(readfile_option==READBUFFER){
    stream->fileinfo = File2Buffer(file);
    if(stream->fileinfo!=NULL&&file2!=NULL){
      bufferstreamdata streaminfo2, *stream2 = &streaminfo2;

      stream2->fileinfo = File2Buffer(file2);
      if(stream2->fileinfo!=NULL){
        AppendFileBuffer(stream->fileinfo, stream2->fileinfo);
      }
      FreeFileBuffer(stream2->fileinfo);
    }
  }
#endif

  STOP_TIMER(read_time);
  STOP_TIMER(read_time_elapsed);

  npropinfo=1; // the 0'th prop is the default human property
  navatar_colors=0;
  FREEMEMORY(avatar_colors);

  FREEMEMORY(geomdiaginfo);
  ngeomdiaginfo = 0;

  FREEMEMORY(treeinfo);
  ntreeinfo=0;
  for(i=0;i<nterraininfo;i++){
    terraindata *terri;

    terri = terraininfo + i;
    FREEMEMORY(terri->x);
    FREEMEMORY(terri->y);
    FREEMEMORY(terri->zcell);
    FREEMEMORY(terri->znode);
//    FREEMEMORY(terri->znormal);
  }
  FREEMEMORY(terraininfo);
  nterraininfo=0;
  niso_compressed=0;
  if(sphereinfo==NULL){
    NewMemory((void **)&sphereinfo,sizeof(spherepoints));
    InitSpherePoints(sphereinfo,14);
  }
  if(wui_sphereinfo==NULL){
    NewMemory((void **)&wui_sphereinfo,sizeof(spherepoints));
    InitSpherePoints(wui_sphereinfo,14);
  }

  ntotal_blockages=0;

  if(ncsvinfo>0){
    csvdata *csvi;

    for(i=0;i<ncsvinfo;i++){
      csvi = csvinfo + i;
      FREEMEMORY(csvi->file);
    }
    FREEMEMORY(csvinfo);
  }
  ncsvinfo=0;

  if(ngeominfo>0){
    for(i=0;i<ngeominfo;i++){
      geomdata *geomi;

      geomi = geominfo + i;
      if(geomi->ngeomobjinfo>0){
        FREEMEMORY(geomi->geomobjinfo);
        geomi->ngeomobjinfo=0;
      }
      FREEMEMORY(geomi->file);
    }
    FREEMEMORY(geominfo);
    ngeominfo=0;
  }

  FREEMEMORY(tickinfo);
  ntickinfo=0;
  ntickinfo_smv=0;

  FREEMEMORY(camera_external);
  if(file!=NULL)NewMemory((void **)&camera_external,sizeof(cameradata));

  FREEMEMORY(camera_external_save);
  if(file!=NULL)NewMemory((void **)&camera_external_save,sizeof(cameradata));

  FREEMEMORY(camera_ini);
  if(file!=NULL){
    NewMemory((void **)&camera_ini,sizeof(cameradata));
    camera_ini->defined=0;
  }

  FREEMEMORY(camera_current);
  if(file!=NULL)NewMemory((void **)&camera_current,sizeof(cameradata));

  FREEMEMORY(camera_internal);
  if(file!=NULL)NewMemory((void **)&camera_internal,sizeof(cameradata));

  FREEMEMORY(camera_save);
  if(file!=NULL)NewMemory((void **)&camera_save,sizeof(cameradata));

  FREEMEMORY(camera_last);
  if(file!=NULL)NewMemory((void **)&camera_last,sizeof(cameradata));

  updatefaces=1;
  nfires=0;
  nrooms=0;

  if(file!=NULL){
    InitSurface(&sdefault);
    NewMemory((void **)&sdefault.surfacelabel,(5+1));
    strcpy(sdefault.surfacelabel,"INERT");

    InitVentSurface(&v_surfacedefault);
    NewMemory((void **)&v_surfacedefault.surfacelabel,(4+1));
    strcpy(v_surfacedefault.surfacelabel,"VENT");

    InitSurface(&e_surfacedefault);
    NewMemory((void **)&e_surfacedefault.surfacelabel,(8+1));
    strcpy(e_surfacedefault.surfacelabel,"EXTERIOR");
    e_surfacedefault.color=mat_ambient2;
  }

  // free memory for particle class

  if(partclassinfo!=NULL){
    int j;

    for(i=0;i<npartclassinfo+1;i++){
      partclassdata *partclassi;

      partclassi = partclassinfo + i;
      FREEMEMORY(partclassi->name);
      if(partclassi->ntypes>0){
        for(j=0;j<partclassi->ntypes;j++){
          flowlabels *labelj;

          labelj = partclassi->labels+j;
          FreeLabels(labelj);
        }
        FREEMEMORY(partclassi->labels);
        partclassi->ntypes=0;
      }
    }
    FREEMEMORY(partclassinfo);
  }
  npartclassinfo=0;

  FreeAllObjects();
  if(ndeviceinfo>0){
    for(i=0;i<ndeviceinfo;i++){
    }
    FREEMEMORY(deviceinfo);
    ndeviceinfo=0;
  }

  // read in device (.svo) definitions

  InitObjectDefs();
  {
    int return_code;

  // get input file name

    return_code=GetInpf(file,file2);
    if(return_code!=0)return return_code;
  }

  if(noutlineinfo>0){
    for(i=0;i<noutlineinfo;i++){
      outlinedata *outlinei;

      outlinei = outlineinfo + i;
      FREEMEMORY(outlinei->x1);
      FREEMEMORY(outlinei->y1);
      FREEMEMORY(outlinei->z1);
      FREEMEMORY(outlinei->x2);
      FREEMEMORY(outlinei->y2);
      FREEMEMORY(outlinei->z2);
    }
    FREEMEMORY(outlineinfo);
    noutlineinfo=0;
  }

  if(nzoneinfo>0){
    for(i=0;i<nzoneinfo;i++){
      zonedata *zonei;
      int n;

      zonei = zoneinfo + i;
      for(n=0;n<4;n++){
        FreeLabels(&zonei->label[n]);
      }
      FREEMEMORY(zonei->file);
    }
    FREEMEMORY(zoneinfo);
  }
  nzoneinfo=0;

  if(nsmoke3dinfo>0){
    {
      smoke3ddata *smoke3di;

      for(i=0;i<nsmoke3dinfo;i++){
        smoke3di = smoke3dinfo + i;
        FreeSmoke3d(smoke3di);
        FREEMEMORY(smoke3di->comp_file);
        FREEMEMORY(smoke3di->reg_file);
      }
      FREEMEMORY(smoke3dinfo);
      nsmoke3dinfo=0;
    }
  }

  if(npartinfo>0){
    for(i=0;i<npartinfo;i++){
      FREEMEMORY(partinfo[i].partclassptr);
      FREEMEMORY(partinfo[i].reg_file);
      FREEMEMORY(partinfo[i].comp_file);
      FREEMEMORY(partinfo[i].size_file);
    }
    FREEMEMORY(partinfo);
  }
  npartinfo=0;


  //*** free slice data

  FREEMEMORY(surfinfo);
  if(nsliceinfo>0){
    for(i=0;i<nsliceinfo;i++){
      slicedata *sd;
      sd = sliceinfo + i;
      FreeLabels(&sliceinfo[i].label);
      FREEMEMORY(sd->reg_file);
      FREEMEMORY(sd->comp_file);
      FREEMEMORY(sd->size_file);
    }
    FREEMEMORY(sliceorderindex);
    for(i=0;i<nmultisliceinfo;i++){
      multislicedata *mslicei;

      mslicei = multisliceinfo + i;
      FREEMEMORY(mslicei->islices);
    }
    FREEMEMORY(multisliceinfo);
    nmultisliceinfo=0;
    FREEMEMORY(sliceinfo);
  }
  nsliceinfo=0;

  //*** free multi-vector slice data

  if(nvsliceinfo>0){
    FREEMEMORY(vsliceorderindex);
    for(i=0;i<nmultivsliceinfo;i++){
      multivslicedata *mvslicei;

      mvslicei = multivsliceinfo + i;
      FREEMEMORY(mvslicei->ivslices);
    }
    FREEMEMORY(multivsliceinfo);
    nmultivsliceinfo=0;
  }

  if(npatchinfo>0){
    for(i=0;i<npatchinfo;i++){
      patchdata *patchi;

      patchi = patchinfo + i;
      FreeLabels(&patchi->label);
      FREEMEMORY(patchi->reg_file);
      FREEMEMORY(patchi->comp_file);
      FREEMEMORY(patchi->size_file);
    }
    FREEMEMORY(patchinfo);
  }
  npatchinfo=0;

  if(nisoinfo>0){
    for(i=0;i<nisoinfo;i++){
      FreeLabels(&isoinfo[i].surface_label);
      FREEMEMORY(isoinfo[i].file);
    }
    FREEMEMORY(isoinfo);
  }
  nisoinfo=0;

  FreeCADInfo();

  updateindexcolors=0;
  ntrnx=0;
  ntrny=0;
  ntrnz=0;
  nmeshes=0;
  npdim=0;
  nVENT=0;
  nCVENT=0;
  ncvents=0;
  nOBST=0;
  noffset=0;
  nsurfinfo=0;
  nmatlinfo=1;
  nvent_transparent=0;

  nvents=0;
  setPDIM=0;
  endian_smv = GetEndian();
  endian_native = GetEndian();
  endian_data=endian_native;
  FREEMEMORY(LESsystem);
  FREEMEMORY(LESendian);

  FREEMEMORY(database_filename);

  FREEMEMORY(vsliceinfo);
  FREEMEMORY(sliceinfo);
  FREEMEMORY(slicetypes);
  FREEMEMORY(vslicetypes);

  FREEMEMORY(plot3dinfo);
  FREEMEMORY(patchinfo);
  FREEMEMORY(boundarytypes);
  FREEMEMORY(isoinfo);
  FREEMEMORY(isotypes);
  FREEMEMORY(roominfo);
  FREEMEMORY(fireinfo);
  FREEMEMORY(zoneinfo);
  FREEMEMORY(zventinfo);

  FREEMEMORY(textureinfo);
  FREEMEMORY(surfinfo);
  FREEMEMORY(terrain_texture);

  if(cadgeominfo!=NULL)FreeCADInfo();

  if(file==NULL){
    InitVars();
    return -1;  // finished  unloading memory from previous case
  }

  if(NewMemory((void **)&LESsystem,4)==0)return 2;
  STRCPY(LESsystem,"");
  if(NewMemory((void **)&LESendian,4)==0)return 2;
  STRCPY(LESendian,"");

#ifdef pp_READBUFFER
  if(readfile_option==READFILE){
    stream->stream1 = fopen(file, "r");
    stream->stream2 = NULL;
    if(stream->stream1==NULL)return 1;
    stream->stream=stream->stream1;
    if(file2!=NULL){
      stream->stream2 = fopen(file2, "r");
      if(stream->stream2==NULL){
        fclose(stream->stream1);
        return 1;
      }
    }
  }
#else
  stream1 = fopen(file, "r");
  if(stream1==NULL)return 1;
  if(file2!=NULL){
    stream2 = fopen(file2, "r");
    if(stream2==NULL){
      fclose(stream1);
      return 1;
    }
  }
  stream=stream1;
#endif

  smv_modtime= FileModtime(file);

  PRINTF(_("processing smokeview file:"));
  PRINTF(" %s\n", file);
  STOP_TIMER(pass0_time );

/*
   ************************************************************************
   ************************ start of pass 1 *******************************
   ************************************************************************
 */

  START_TIMER(pass1_time);

  nvents=0;
  igrid=0;
  ioffset=0;
  ntc_total=0;
  nspr_total=0;
  nheat_total=0;
  PRINTF("%s","  pass 1\n");
  for(;;){
    if(FEOF(stream)!=0){
      BREAK;
    }
    if(FGETS(buffer,255,stream)==NULL){
      BREAK;
    }
    TrimBack(buffer);
    if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;

    /*
      The keywords TRNX, TRNY, TRNZ, GRID, PDIM, OBST and VENT are not required
      BUT if any one these keywords are present then the number of each MUST be equal
    */


    if(Match(buffer, "SOLID_HT3D")==1){
      FGETS(buffer, 255, stream);
      sscanf(buffer, "%i", &solid_ht3d);
      ONEORZERO(solid_ht3d);
      if(solid_ht3d==1)show_slice_in_obst=GAS_AND_SOLID;
      continue;
    }
    if(Match(buffer, "IBLANK")==1){
      FGETS(buffer, 255, stream);
      if(iblank_set_on_commandline==0){
        sscanf(buffer, "%i", &use_iblank);
        use_iblank = CLAMP(use_iblank, 0, 1);
      }
      continue;
    }
    if(Match(buffer,"GVEC") == 1){
      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f",gvecphys,gvecphys+1,gvecphys+2);
      gvecunit[0]=gvecphys[0];
      gvecunit[1]=gvecphys[1];
      gvecunit[2]=gvecphys[2];
      NORMALIZE3(gvecunit);
      if(NORM3(gvecphys)>0.0){
        have_gvec=1;
      }
      continue;
    }
    if(Match(buffer,"CSVF") == 1){
      int nfiles;
      char *file_ptr,*type_ptr;

      FGETS(buffer,255,stream);
      TrimBack(buffer);
      type_ptr=TrimFront(buffer);

      FGETS(buffer2,255,stream);
      TrimBack(buffer2);
      file_ptr=TrimFront(buffer2);
      nfiles=1;
      if(strcmp(type_ptr,"hrr")==0){
        if(FILE_EXISTS_CASEDIR(file_ptr)==NO)nfiles=0;
      }
      else if(strcmp(type_ptr,"devc")==0){
        if(FILE_EXISTS_CASEDIR(file_ptr)==NO)nfiles=0;
      }
      else if(strcmp(type_ptr,"ext")==0){
        if(strchr(file_ptr,'*')==NULL){
          if(FILE_EXISTS_CASEDIR(file_ptr)==NO)nfiles=0;
        }
        else{
          nfiles = GetFileListSize(".",file_ptr);
        }
      }
      else{
        nfiles=0;
      }
      ncsvinfo+=nfiles;
      continue;
    }
    if(Match(buffer, "GEOMDIAG") == 1){
      ngeomdiaginfo++;
      continue;
    }
    if(Match(buffer, "GEOM") == 1 ||
       Match(buffer, "BGEOM") == 1 ||
       Match(buffer, "SGEOM") == 1){
      ngeominfo++;
      continue;
    }
    if(Match(buffer,"PROP") == 1){
      npropinfo++;
      continue;
    }
    if(Match(buffer,"SMOKEDIFF") == 1){
      smokediff=1;
      continue;
    }
    if(Match(buffer,"ALBEDO") == 1){
      FGETS(buffer,255,stream);
      sscanf(buffer,"%f",&smoke_albedo);
      smoke_albedo = CLAMP(smoke_albedo, 0.0, 1.0);
      smoke_albedo_base = smoke_albedo;
      continue;
    }
    if(Match(buffer, "NORTHANGLE")==1){
      FGETS(buffer, 255, stream);
      sscanf(buffer, "%f", &northangle);
      northangle = CLAMP(northangle, -180.0, 180.0);
      have_northangle = 1;
      continue;
    }
    if(Match(buffer,"AVATAR_COLOR")==1){
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&navatar_colors);
      if(navatar_colors<0)navatar_colors=0;
      if(navatar_colors>0){
        float *acolor;

        NewMemory((void **)&avatar_colors,3*navatar_colors*sizeof(float));
        acolor=avatar_colors;
        for(i=0;i<navatar_colors;i++){
          int irgb[3];
          FGETS(buffer,255,stream);
          irgb[0]=0;
          irgb[1]=0;
          irgb[2]=0;
          sscanf(buffer,"%i %i %i",irgb,irgb+1,irgb+2);
          acolor[0]=(float)irgb[0]/255.0;
          acolor[1]=(float)irgb[1]/255.0;
          acolor[2]=(float)irgb[2]/255.0;
          acolor+=3;
        }
      }
      continue;
    }
    if(Match(buffer,"TERRAIN") == 1){
      nterraininfo++;
      continue;
    }
    if(Match(buffer,"CLASS_OF_PARTICLES") == 1||
       Match(buffer,"CLASS_OF_HUMANS") == 1){
      npartclassinfo++;
      continue;
    }
    if(Match(buffer,"AUTOTERRAIN") == 1){
      int len_buffer;
      texturedata *tt;
      char *buff2;

      NewMemory((void **)&terrain_texture,sizeof(texturedata));
      tt = terrain_texture;
      autoterrain=1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&visTerrainType);
      visTerrainType=CLAMP(visTerrainType,0,4);
      if(visTerrainType==TERRAIN_HIDDEN){
        if(visOtherVents!=visOtherVentsSAVE)visOtherVents=visOtherVentsSAVE;
      }
      else{
        if(visOtherVents!=0){
          visOtherVentsSAVE=visOtherVents;
          visOtherVents=0;
        }
      }


      FGETS(buffer,255,stream);
      buff2 = TrimFront(buffer);
      TrimBack(buff2);
      len_buffer = strlen(buff2);
      if(len_buffer>0&&strcmp(buff2, "xxxnull")!=0){

        NewMemory((void **)&tt->file, (len_buffer+1)*sizeof(char));
        strcpy(tt->file, buff2);
      }
      else{
        tt->file = NULL;
      }
      continue;
    }
    if(
      (Match(buffer,"DEVICE") == 1)&&
      (Match(buffer,"DEVICE_ACT") != 1)
      ){
      FGETS(buffer,255,stream);
      FGETS(buffer,255,stream);
      ndeviceinfo++;
      continue;
    }
    if(
        Match(buffer,"SPRK") == 1||
       Match(buffer,"HEAT") == 1||
       Match(buffer,"SMOD") == 1||
       Match(buffer,"THCP") == 1
    ){
      int local_ntc;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&local_ntc);
      if(local_ntc<0)local_ntc=0;
      ndeviceinfo+=local_ntc;
      for(i=0;i<local_ntc;i++){
        FGETS(buffer,255,stream);
      }
      continue;
    }
    if(Match(buffer,"FDSVERSION")==1){
      int lenbuffer;
      char *buffptr;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      TrimBack(buffer);
      buffptr = TrimFront(buffer);
      lenbuffer = strlen(buffptr);
      if(lenbuffer>0){
        NewMemory((void **)&fds_version,lenbuffer+1);
        strcpy(fds_version,buffer);
      }
      else{
        NewMemory((void **)&fds_version,7+1);
        strcpy(fds_version,"unknown");
      }

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      TrimBack(buffer);
      buffptr = TrimFront(buffer);
      lenbuffer = strlen(buffptr);
      if(lenbuffer>0){
        NewMemory((void **)&fds_githash,lenbuffer+1);
        strcpy(fds_githash,buffer);
      }
      else{
        NewMemory((void **)&fds_githash,7+1);
        strcpy(fds_githash,"unknown");
      }
      continue;
    }
    if(Match(buffer,"TOFFSET")==1){
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      sscanf(buffer,"%f %f %f",texture_origin,texture_origin+1,texture_origin+2);
      continue;
    }

    if(Match(buffer,"USETEXTURES") == 1){
      usetextures=1;
      continue;
    }

    if(Match(buffer,"CADTEXTUREPATH") == 1||
       Match(buffer,"TEXTUREDIR") == 1){
         if(FGETS(buffer,255,stream)==NULL){
           BREAK;
         }
      TrimBack(buffer);
      {
        size_t texturedirlen;

        texturedirlen=strlen(TrimFront(buffer));
        if(texturedirlen>0){
          FREEMEMORY(texturedir);
          NewMemory( (void **)&texturedir,texturedirlen+1);
          strcpy(texturedir,TrimFront(buffer));
        }
      }
      continue;
    }

    if(Match(buffer,"VIEWTIMES") == 1){
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      sscanf(buffer,"%f %f %i",&view_tstart,&view_tstop,&view_ntimes);
      if(view_ntimes<2)view_ntimes=2;
      ReallocTourMemory();
      continue;
    }
    if(Match(buffer,"OUTLINE") == 1){
      noutlineinfo++;
      continue;
    }
    if(Match(buffer,"TICKS") == 1){
      ntickinfo++;
      ntickinfo_smv++;
      continue;
    }
    if(Match(buffer,"TRNX") == 1){
      ntrnx++;
      continue;
    }
    if(Match(buffer,"TRNY") == 1){
      ntrny++;
      continue;
    }
    if(Match(buffer,"TRNZ") == 1){
      ntrnz++;
      continue;
    }
    if(Match(buffer,"SURFACE") ==1){
      nsurfinfo++;
      continue;
    }
    if(Match(buffer,"MATERIAL") ==1){
      nmatlinfo++;
      continue;
    }
    if(Match(buffer,"GRID") == 1){
      noGRIDpresent=0;
      nmeshes++;
      continue;
    }
    if(Match(buffer,"OFFSET") == 1){
      noffset++;
      continue;
    }
    if(Match(buffer,"PDIM") == 1){
      npdim++;
      setPDIM=1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f %f %f %f",&xbar0,&xbar,&ybar0,&ybar,&zbar0,&zbar);
      continue;
    }
    if(Match(buffer,"OBST") == 1){
      nOBST++;
      continue;
    }
    if(Match(buffer,"CADGEOM") == 1){
      ncadgeom++;
      continue;
    }
    if(Match(buffer,"CVENT") == 1){
      nCVENT++;
      continue;
    }
    if(Match(buffer,"VENT") == 1){
      nVENT++;
      continue;
    }
    if(Match(buffer,"PRT5")==1||Match(buffer,"EVA5")==1
      ){
      if(setup_only == 1)continue;
      npartinfo++;
      continue;
    }
    if( (Match(buffer,"SLCF") == 1)||
        (Match(buffer,"SLCC") == 1)||
        (Match(buffer, "SLCD") == 1) ||
        (Match(buffer, "SLFL") == 1) ||
        (Match(buffer,"SLCT") == 1)
      ){
      if(setup_only == 1)continue;
      nsliceinfo++;
      nslicefiles=nsliceinfo;
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      continue;
    }
    if(
      Match(buffer,"SMOKE3D") == 1||
      Match(buffer,"VSMOKE3D") == 1||
      Match(buffer,"SMOKF3D") == 1||
      Match(buffer,"VSMOKF3D") == 1
      ||Match(buffer, "SMOKG3D") == 1 ||
      Match(buffer, "VSMOKG3D") == 1
      ){
      if(setup_only==1)continue;
      nsmoke3dinfo++;
      continue;
    }
    if(
      Match(buffer,"MINMAXBNDF") == 1||
      Match(buffer,"MINMAXPL3D") == 1||
      Match(buffer,"MINMAXSLCF") == 1
      ){
      do_pass4=1;
      continue;
    }
    if(Match(buffer, "BNDF") == 1 || Match(buffer, "BNDC") == 1 || Match(buffer, "BNDE") == 1 || Match(buffer, "BNDS") == 1){
      if(setup_only == 1)continue;
      npatchinfo++;
      continue;
    }
    if(Match(buffer,"ISOF") == 1||Match(buffer,"TISOF")==1||Match(buffer,"ISOG") == 1){
      if(setup_only == 1)continue;
      nisoinfo++;
      continue;
    }
    if(Match(buffer,"ROOM") == 1){
      isZoneFireModel=1;
      nrooms++;
      continue;
    }
    if(Match(buffer,"FIRE") == 1){
      nfires++;
      continue;
    }
    if(Match(buffer,"ZONE") == 1){
      nzoneinfo++;
      continue;
    }
    if(Match(buffer, "VENTGEOM")==1||Match(buffer, "HFLOWGEOM")==1||Match(buffer, "VFLOWGEOM")==1||Match(buffer, "MFLOWGEOM")==1){
      nzvents++;
      continue;
    }
    if(Match(buffer, "HVENTGEOM")==1||Match(buffer, "VVENTGEOM")==1||Match(buffer, "MVENTGEOM")==1){
      nzventsnew++;
      continue;
    }
    if(Match(buffer, "HVENTPOS") == 1 || Match(buffer, "VVENTPOS") == 1 || Match(buffer, "MVENTPOS") == 1){
      nzventsnew++;
      continue;
    }

  }

  STOP_TIMER(pass1_time);


/*
   ************************************************************************
   ************************ end of pass 1 *********************************
   ************************************************************************
 */

  START_TIMER(pass2_time);

 if(fds_version==NULL){
   NewMemory((void **)&fds_version,7+1);
   strcpy(fds_version,"unknown");
 }
 if(fds_githash==NULL){
   NewMemory((void **)&fds_githash,7+1);
   strcpy(fds_githash,"unknown");
 }
 if(nisoinfo>0&&nmeshes>0)nisos_per_mesh = MAX(nisoinfo / nmeshes,1);
 if(ncsvinfo > 0){
   NewMemory((void **)&csvinfo,ncsvinfo*sizeof(csvdata));
   ncsvinfo=0;
 }
 if(ngeominfo>0){
   NewMemory((void **)&geominfo,ngeominfo*sizeof(geomdata));
   ngeominfo=0;
 }
 if(npropinfo>0){
   NewMemory((void **)&propinfo,npropinfo*sizeof(propdata));
   npropinfo=1; // the 0'th prop is the default human property
 }
 if(nterraininfo>0){
   NewMemory((void **)&terraininfo,nterraininfo*sizeof(terraindata));
   nterraininfo=0;
 }
 if(npartclassinfo>=0){
   float rgb_class[4];
   partclassdata *partclassi;
   size_t len;

   NewMemory((void **)&partclassinfo,(npartclassinfo+1)*sizeof(partclassdata));

   // define a dummy class

   partclassi = partclassinfo + npartclassinfo;
   strcpy(buffer,"Default");
   TrimBack(buffer);
   len=strlen(buffer);
   partclassi->name=NULL;
   if(len>0){
     NewMemory((void **)&partclassi->name,len+1);
     STRCPY(partclassi->name,TrimFront(buffer));
   }

   rgb_class[0]=1.0;
   rgb_class[1]=0.0;
   rgb_class[2]=0.0;
   rgb_class[3]=1.0;
   partclassi->rgb=GetColorPtr(rgb_class);

   partclassi->ntypes=0;
   partclassi->xyz=NULL;
   partclassi->maxpoints=0;
   partclassi->labels=NULL;

   NewMemory((void **)&partclassi->labels,sizeof(flowlabels));
   CreateNullLabel(partclassi->labels);

   npartclassinfo=0;


 }

  ibartemp=2;
  jbartemp=2;
  kbartemp=2;

  /* --------- set up multi-block data structures ------------- */

  /*
     The keywords TRNX, TRNY, TRNZ, GRID, PDIM, OBST and VENT are not required
     BUT if any one is present then the number of each must be equal
  */

  if(nmeshes==0&&ntrnx==0&&ntrny==0&&ntrnz==0&&npdim==0&&nOBST==0&&nVENT==0&&noffset==0){
    nmeshes=1;
    ntrnx=1;
    ntrny=1;
    ntrnz=1;
    npdim=1;
    nOBST=1;
    noffset=1;
  }
  else{
    if(nmeshes>1){
      if((nmeshes!=ntrnx||nmeshes!=ntrny||nmeshes!=ntrnz||nmeshes!=npdim||nmeshes!=nOBST||nmeshes!=nVENT||nmeshes!=noffset)&&
         (nCVENT!=0&&nCVENT!=nmeshes)){
        fprintf(stderr,"*** Error:\n");
        if(nmeshes!=ntrnx)fprintf(stderr,"*** Error:  found %i TRNX keywords, was expecting %i\n",ntrnx,nmeshes);
        if(nmeshes!=ntrny)fprintf(stderr,"*** Error:  found %i TRNY keywords, was expecting %i\n",ntrny,nmeshes);
        if(nmeshes!=ntrnz)fprintf(stderr,"*** Error:  found %i TRNZ keywords, was expecting %i\n",ntrnz,nmeshes);
        if(nmeshes!=npdim)fprintf(stderr,"*** Error:  found %i PDIM keywords, was expecting %i\n",npdim,nmeshes);
        if(nmeshes!=nOBST)fprintf(stderr,"*** Error:  found %i OBST keywords, was expecting %i\n",nOBST,nmeshes);
        if(nmeshes!=nVENT)fprintf(stderr,"*** Error:  found %i VENT keywords, was expecting %i\n",nVENT,nmeshes);
        if(nCVENT!=0&&nmeshes!=nCVENT)fprintf(stderr,"*** Error:  found %i CVENT keywords, was expecting %i\n",noffset,nmeshes);
        return 2;
      }
    }
  }
  FREEMEMORY(meshinfo);
  if(NewMemory((void **)&meshinfo,nmeshes*sizeof(meshdata))==0)return 2;
  FREEMEMORY(supermeshinfo);
  if(NewMemory((void **)&supermeshinfo,nmeshes*sizeof(supermeshdata))==0)return 2;
  meshinfo->plot3dfilenum=-1;
  UpdateCurrentMesh(meshinfo);
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;
    supermeshdata *smeshi;

    smeshi = supermeshinfo + i;
    smeshi->nmeshes=0;

    meshi=meshinfo+i;
    meshi->ibar=0;
    meshi->jbar=0;
    meshi->kbar=0;
    meshi->nbptrs=0;
    meshi->nvents=0;
    meshi->ncvents=0;
    meshi->plotn=1;
    meshi->itextureoffset=0;
  }
  if(setPDIM==0){
    meshdata *meshi;

    if(roomdefined==0){
      xbar0 = 0.0;    xbar = 1.0;
      ybar0 = 0.0;    ybar = 1.0;
      zbar0 = 0.0;    zbar = 1.0;
    }
    meshi=meshinfo;
    meshi->xyz_bar0[XXX]=xbar0;
    meshi->xyz_bar[XXX] =xbar;
    meshi->xcen=(xbar+xbar0)/2.0;
    meshi->xyz_bar0[YYY]=ybar0;
    meshi->xyz_bar[YYY] =ybar;
    meshi->ycen=(ybar+ybar0)/2.0;
    meshi->xyz_bar0[ZZZ]=zbar0;
    meshi->xyz_bar[ZZZ] =zbar;
    meshi->zcen=(zbar+zbar0)/2.0;
  }

  // define labels and memory for default colorbars

  FREEMEMORY(partinfo);
  if(npartinfo!=0){
    if(NewMemory((void **)&partinfo,npartinfo*sizeof(partdata))==0)return 2;
  }

  FREEMEMORY(vsliceinfo);
  FREEMEMORY(sliceinfo);
  FREEMEMORY(slicetypes);
  FREEMEMORY(vslicetypes);
  FREEMEMORY(fedinfo);
  if(nsliceinfo>0){
    if(NewMemory( (void **)&vsliceinfo, 3*nsliceinfo*sizeof(vslicedata) )==0||
       NewMemory( (void **)&sliceinfo,  nsliceinfo*sizeof(slicedata)    )==0||
       NewMemory( (void **)&fedinfo,  nsliceinfo*sizeof(feddata)    )==0||
       NewMemory( (void **)&slicetypes, nsliceinfo*sizeof(int)      )==0||
       NewMemory( (void **)&slice_loadstack, nsliceinfo*sizeof(int)      )==0||
       NewMemory( (void **)&vslice_loadstack, nsliceinfo*sizeof(int)      )==0||
       NewMemory( (void **)&subslice_menuindex, nsliceinfo*sizeof(int)      )==0||
       NewMemory((void **)&msubslice_menuindex, nsliceinfo*sizeof(int))==0||
       NewMemory((void **)&subvslice_menuindex, nsliceinfo*sizeof(int))==0||
       NewMemory((void **)&msubvslice_menuindex, nsliceinfo*sizeof(int))==0||
       NewMemory((void **)&mslice_loadstack, nsliceinfo*sizeof(int))==0||
       NewMemory( (void **)&mvslice_loadstack, nsliceinfo*sizeof(int)      )==0||
       NewMemory( (void **)&vslicetypes,3*nsliceinfo*sizeof(int)    )==0){
       return 2;
    }
    sliceinfo_copy=sliceinfo;
    nslice_loadstack=nsliceinfo;
    islice_loadstack=0;
    nvslice_loadstack=nsliceinfo;
    ivslice_loadstack=0;
    nmslice_loadstack=nsliceinfo;
    imslice_loadstack=0;
    nmvslice_loadstack=nsliceinfo;
    imvslice_loadstack=0;

  }
  if(nsmoke3dinfo>0){
    if(NewMemory( (void **)&smoke3dinfo, nsmoke3dinfo*sizeof(smoke3ddata))==0)return 2;
  }

  FREEMEMORY(patchinfo);
  FREEMEMORY(boundarytypes);
  if(npatchinfo!=0){
    if(NewMemory((void **)&patchinfo,npatchinfo*sizeof(patchdata))==0)return 2;
    for(i=0;i<npatchinfo;i++){
      patchdata *patchi;

      patchi = patchinfo + i;
      patchi->reg_file=NULL;
      patchi->comp_file=NULL;
      patchi->file=NULL;
      patchi->size_file=NULL;
    }
    if(NewMemory((void **)&boundarytypes,npatchinfo*sizeof(int))==0)return 2;
  }
  FREEMEMORY(isoinfo);
  FREEMEMORY(isotypes);
  if(nisoinfo>0){
    if(NewMemory((void **)&isoinfo,nisoinfo*sizeof(isodata))==0)return 2;
    if(NewMemory((void **)&isotypes,nisoinfo*sizeof(int))==0)return 2;
  }
  FREEMEMORY(roominfo);
  if(nrooms>0){
    if(NewMemory((void **)&roominfo,(nrooms+1)*sizeof(roomdata))==0)return 2;
  }
  FREEMEMORY(fireinfo);
  if(nfires>0){
    if(NewMemory((void **)&fireinfo,nfires*sizeof(firedata))==0)return 2;
  }
  FREEMEMORY(zoneinfo);
  if(nzoneinfo>0){
    if(NewMemory((void **)&zoneinfo,nzoneinfo*sizeof(zonedata))==0)return 2;
  }
  FREEMEMORY(zventinfo);
  if(nzventsnew>0)nzvents=nzventsnew;
  if(nzvents>0){
    if(NewMemory((void **)&zventinfo,nzvents*sizeof(zventdata))==0)return 2;
  }
  nzvents=0;
  nzhvents=0;
  nzvvents=0;
  nzmvents = 0;

  FREEMEMORY(textureinfo);
  FREEMEMORY(surfinfo);
  if(NewMemory((void **)&surfinfo,(nsurfinfo+MAX_ISO_COLORS+1)*sizeof(surfdata))==0)return 2;

  {
    matldata *matli;
    float s_color[4];

    FREEMEMORY(matlinfo);
    if(NewMemory((void **)&matlinfo,nmatlinfo*sizeof(matldata))==0)return 2;
    matli = matlinfo;
    InitMatl(matli);
    s_color[0]=matli->color[0];
    s_color[1]=matli->color[1];
    s_color[2]=matli->color[2];
    s_color[3]=matli->color[3];
    matli->color = GetColorPtr(s_color);
  }

  if(cadgeominfo!=NULL)FreeCADInfo();
  if(ncadgeom>0){
    if(NewMemory((void **)&cadgeominfo,ncadgeom*sizeof(cadgeomdata))==0)return 2;
  }

  if(noutlineinfo>0){
    if(NewMemory((void **)&outlineinfo,noutlineinfo*sizeof(outlinedata))==0)return 2;
    for(i=0;i<noutlineinfo;i++){
      outlinedata *outlinei;

      outlinei = outlineinfo + i;
      outlinei->x1=NULL;
      outlinei->x2=NULL;
      outlinei->y1=NULL;
      outlinei->y2=NULL;
      outlinei->z1=NULL;
      outlinei->z2=NULL;
    }
  }
  if(ntickinfo>0){
    if(NewMemory((void **)&tickinfo,ntickinfo*sizeof(tickdata))==0)return 2;
    ntickinfo=0;
    ntickinfo_smv=0;
  }

  if(npropinfo>0){
    npropinfo=0;
    InitDefaultProp();
    npropinfo=1;
  }

  if(ngeomdiaginfo > 0){
    NewMemory((void **)&geomdiaginfo, ngeomdiaginfo*sizeof(geomdiagdata));
    ngeomdiaginfo = 0;
  }

/*
   ************************************************************************
   ************************ start of pass 2 *******************************
   ************************************************************************
 */

  startpass=1;
  ioffset=0;
  iobst=0;
  ncadgeom=0;
  nsurfinfo=0;
  nmatlinfo=1;
  noutlineinfo=0;
  if(noffset==0)ioffset=1;

#ifdef pp_READBUFFER
  REWIND(stream);
#else
  REWIND(stream1);
  if(stream2!=NULL)rewind(stream2);
  stream=stream1;
#endif
  PRINTF("%s","  pass 2\n");
  for(;;){
    if(FEOF(stream)!=0){
      BREAK;
    }
    if(noGRIDpresent==1&&startpass==1){
      strcpy(buffer,"GRID");
      startpass=0;
    }
    else{
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      TrimBack(buffer);
      if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;
    }
    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ CSVF ++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"CSVF") == 1){
      csvdata *csvi;
      char *type_ptr, *file_ptr;
      int nfiles=1;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      TrimBack(buffer);
      type_ptr=TrimFront(buffer);

      if(FGETS(buffer2,255,stream)==NULL){
        BREAK;
      }
      TrimBack(buffer2);
      file_ptr=TrimFront(buffer2);
      nfiles=1;
      if(strcmp(type_ptr,"hrr")==0){
        if(FILE_EXISTS_CASEDIR(file_ptr)==NO)nfiles=0;
      }
      else if(strcmp(type_ptr,"devc")==0){
        if(FILE_EXISTS_CASEDIR(file_ptr)==NO)nfiles=0;
      }
      else if(strcmp(type_ptr,"ext")==0){
        if(strchr(file_ptr,'*')==NULL){
          if(FILE_EXISTS_CASEDIR(file_ptr)==NO)nfiles=0;
        }
        else{
          nfiles = GetFileListSize(".",file_ptr);
        }
      }
      else{
        nfiles=0;
      }
      if(nfiles==0)continue;

      csvi = csvinfo + ncsvinfo;

      csvi->loaded=0;
      csvi->display=0;

      if(strcmp(type_ptr,"hrr")==0){
        if(FILE_EXISTS_CASEDIR(file_ptr)==YES){
          csvi->type=CSVTYPE_HRR;
          NewMemory((void **)&csvi->file,strlen(file_ptr)+1);
          strcpy(csvi->file,file_ptr);
        }
        else{
          nfiles=0;
        }
      }
      else if(strcmp(type_ptr,"devc")==0){
        if(FILE_EXISTS_CASEDIR(file_ptr)==YES){
          csvi->type=CSVTYPE_DEVC;
          NewMemory((void **)&csvi->file,strlen(file_ptr)+1);
          strcpy(csvi->file,file_ptr);
        }
        else{
          nfiles=0;
        }
      }
      else if(strcmp(type_ptr,"ext")==0){
        if(strchr(file_ptr,'*')==NULL){
          if(FILE_EXISTS_CASEDIR(file_ptr)==YES){
            csvi->type=CSVTYPE_EXT;
            NewMemory((void **)&csvi->file,strlen(file_ptr)+1);
            strcpy(csvi->file,file_ptr);
          }
          else{
            nfiles=0;
          }
        }
        else{
          filelistdata *filelist;
          int nfilelist;

          nfilelist = GetFileListSize(".",file_ptr);
          nfiles= MakeFileList(".",file_ptr,nfilelist,NO,&filelist);
          for(i=0;i<nfiles;i++){
            csvi = csvinfo + ncsvinfo + i;
            csvi->loaded=0;
            csvi->display=0;
            csvi->type=CSVTYPE_EXT;
            NewMemory((void **)&csvi->file,strlen(filelist[i].file)+1);
            strcpy(csvi->file,filelist[i].file);
          }
          FreeFileList(filelist,&nfilelist);
        }
      }
      else{
        nfiles=0;
      }

      ncsvinfo+=nfiles;
      continue;
    }

    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ GEOMDIAG ++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    */
    if(Match(buffer, "GEOMDIAG") == 1){
      geomdiagdata *geomdiagi;
      char *buffptr;

      geomdiagi = geomdiaginfo + ngeomdiaginfo;
      ngeomdiaginfo++;

      FGETS(buffer, 255, stream);
      TrimBack(buffer);
      buffptr = TrimFront(buffer);
      NewMemory((void **)&geomdiagi->geomfile, strlen(buffptr) + 1);
      strcpy(geomdiagi->geomfile, buffptr);

      NewMemory((void **)&geomdiagi->geom, sizeof(geomdata));
      InitGeom(geomdiagi->geom,GEOM_GEOM,NOT_FDSBLOCK);

      NewMemory((void **)&geomdiagi->geom->file, strlen(buffptr) + 1);
      strcpy(geomdiagi->geom->file, buffptr);

      FGETS(buffer, 255, stream);
      TrimBack(buffer);
      buffptr = TrimFront(buffer);
      NewMemory((void **)&geomdiagi->geomdatafile, strlen(buffptr) + 1);
      strcpy(geomdiagi->geomdatafile, buffptr);
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ GEOM ++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer, "GEOM") == 1 ||
       Match(buffer, "BGEOM") == 1 ||
       Match(buffer, "SGEOM") == 1){
      geomdata *geomi;
      char *buff2;
      int ngeomobjinfo=0;

      geomi = geominfo + ngeominfo;
      geomi->ngeomobjinfo=0;
      geomi->geomobjinfo=NULL;
      geomi->memory_id = ++nmemory_ids;

      TrimBack(buffer);
      if(strlen(buffer)>4){

        buff2 = buffer+5;
        sscanf(buff2,"%i",&ngeomobjinfo);
      }
      if(Match(buffer, "BGEOM") == 1){
        InitGeom(geomi, GEOM_BOUNDARY, NOT_FDSBLOCK);
      }
      else if(Match(buffer, "SGEOM") == 1){
        InitGeom(geomi, GEOM_SLICE, NOT_FDSBLOCK);
      }
      else{
        InitGeom(geomi, GEOM_GEOM, FDSBLOCK);
      }

      FGETS(buffer,255,stream);
      TrimBack(buffer);
      buff2 = TrimFront(buffer);
      NewMemory((void **)&geomi->file,strlen(buff2)+1);
      strcpy(geomi->file,buff2);

      if(ngeomobjinfo>0){
        NewMemory((void **)&geomi->geomobjinfo,ngeomobjinfo*sizeof(geomobjdata));
        for(i=0;i<ngeomobjinfo;i++){
          geomobjdata *geomobji;
          float *center;
          char *texture_mapping=NULL, *texture_vals=NULL;

          geomobji = geomi->geomobjinfo + i;

          geomobji->texture_name=NULL;
          geomobji->texture_mapping=TEXTURE_RECTANGULAR;

          FGETS(buffer,255,stream);

          texture_mapping = TrimFront(buffer);
          if(texture_mapping!=NULL)texture_vals = strchr(texture_mapping,' ');

          if(texture_vals!=NULL){
            char *surflabel;

            texture_vals++;
            texture_vals[-1]=0;
            center = geomobji->texture_center;
            sscanf(texture_vals,"%f %f %f",center,center+1,center+2);
            surflabel=strchr(texture_vals,'%');
            if(surflabel!=NULL){
              TrimBack(surflabel);
              surflabel=TrimFront(surflabel+1);
              geomi->surf=GetSurface(surflabel);
            }
          }

          if(texture_mapping!=NULL&&strcmp(texture_mapping,"SPHERICAL")==0){
            geomobji->texture_mapping=TEXTURE_SPHERICAL;
          }
        }
      }

      ngeominfo++;
      continue;
    }

    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ OBST ++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OBST") == 1&&autoterrain==1){
      int nobsts=0;
      meshdata *meshi;
      unsigned char *is_block_terrain;
      int nn;

      iobst++;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&nobsts);


      meshi=meshinfo+iobst-1;

      if(nobsts<=0)continue;

      NewMemory((void **)&meshi->is_block_terrain,nobsts*sizeof(unsigned char));
      is_block_terrain=meshi->is_block_terrain;

      for(nn=0;nn<nobsts;nn++){
        FGETS(buffer,255,stream);
      }
      for(nn=0;nn<nobsts;nn++){
        int ijk2[6],colorindex_local=0,blocktype_local=-1;

        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %i %i %i %i %i %i %i",ijk2,ijk2+1,ijk2+2,ijk2+3,ijk2+4,ijk2+5,&colorindex_local,&blocktype_local);
        if(blocktype_local>=0&&(blocktype_local&8)==8){
          is_block_terrain[nn]=1;
        }
        else{
          is_block_terrain[nn]=0;
        }
        // temporary work around for terrain display of slice files
       // if(autoterrain==1)is_block_terrain[nn]=1;
      }
      continue;
    }

    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ PROP ++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"PROP") == 1){
      propdata *propi;
      char *fbuffer;
      char proplabel[255];
      int lenbuf;
      int ntextures_local;
      int nsmokeview_ids;
      char *smokeview_id;

      propi = propinfo + npropinfo;

      if(FGETS(proplabel,255,stream)==NULL){
        BREAK;  // prop label
      }
      TrimBack(proplabel);
      fbuffer=TrimFront(proplabel);

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;  // number of smokeview_id's
      }
      sscanf(buffer,"%i",&nsmokeview_ids);

      InitProp(propi,nsmokeview_ids,fbuffer);
      for(i=0;i<nsmokeview_ids;i++){
        if(FGETS(buffer,255,stream)==NULL){
          BREAK; // smokeview_id
        }
        TrimBack(buffer);
        fbuffer=TrimFront(buffer);
        lenbuf=strlen(fbuffer);
        NewMemory((void **)&smokeview_id,lenbuf+1);
        strcpy(smokeview_id,fbuffer);
        propi->smokeview_ids[i]=smokeview_id;
        propi->smv_objects[i]=GetSmvObjectType(propi->smokeview_ids[i],missing_device);
      }
      propi->smv_object=propi->smv_objects[0];
      propi->smokeview_id=propi->smokeview_ids[0];

      if(FGETS(buffer,255,stream)==NULL){
        BREAK; // keyword_values
      }
      sscanf(buffer,"%i",&propi->nvars_indep);
      propi->vars_indep=NULL;
      propi->svals=NULL;
      propi->texturefiles=NULL;
      ntextures_local=0;
      if(propi->nvars_indep>0){
        NewMemory((void **)&propi->vars_indep,propi->nvars_indep*sizeof(char *));
        NewMemory((void **)&propi->svals,propi->nvars_indep*sizeof(char *));
        NewMemory((void **)&propi->fvals,propi->nvars_indep*sizeof(float));
        NewMemory((void **)&propi->vars_indep_index,propi->nvars_indep*sizeof(int));
        NewMemory((void **)&propi->texturefiles,propi->nvars_indep*sizeof(char *));

        for(i=0;i<propi->nvars_indep;i++){
          char *equal;

          propi->svals[i]=NULL;
          propi->vars_indep[i]=NULL;
          propi->fvals[i]=0.0;
          FGETS(buffer,255,stream);
          equal=strchr(buffer,'=');
          if(equal!=NULL){
            char *buf1, *buf2, *keyword, *val;
            int lenkey, lenval;
            char *texturefile;

            buf1=buffer;
            buf2=equal+1;
            *equal=0;

            TrimBack(buf1);
            keyword=TrimFront(buf1);
            lenkey=strlen(keyword);

            TrimBack(buf2);
            val=TrimFront(buf2);
            lenval=strlen(val);

            if(lenkey==0||lenval==0)continue;

            if(val[0]=='"'){
              val[0]=' ';
              if(val[lenval-1]=='"')val[lenval-1]=' ';
              TrimBack(val);
              val=TrimFront(val);
              NewMemory((void **)&propi->svals[i],lenval+1);
              strcpy(propi->svals[i],val);
              texturefile=strstr(val,"t%");
              if(texturefile!=NULL){
                texturefile+=2;
                texturefile=TrimFront(texturefile);
                propi->texturefiles[ntextures_local]=propi->svals[i];
                strcpy(propi->svals[i],texturefile);

                ntextures_local++;
              }
            }

            NewMemory((void **)&propi->vars_indep[i],lenkey+1);
            strcpy(propi->vars_indep[i],keyword);

            sscanf(val,"%f",propi->fvals+i);
          }
        }
        GetIndepVarIndices(propi->smv_object,propi->vars_indep,propi->nvars_indep,propi->vars_indep_index);
        GetEvacIndices(propi->smv_object,propi->fvars_evac_index,&propi->nvars_evac);

      }
      propi->ntextures=ntextures_local;
      npropinfo++;
      continue;
    }

    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ TERRAIN +++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"TERRAIN") == 1){
//      terraindata *terri;
      float xmin, xmax, ymin, ymax;
      int nx, ny;

      manual_terrain=1;

  //    terri = terraininfo + nterraininfo;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %i %f %f %i",&xmin, &xmax, &nx, &ymin, &ymax, &ny);
      // must implement new form for defining terrain surfaces
      //initterrain(stream, NULL, terri, xmin, xmax, nx, ymin, ymax, ny);

      nterraininfo++;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++ CLASS_OF_PARTICLES +++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"CLASS_OF_PARTICLES") == 1||
       Match(buffer,"CLASS_OF_HUMANS") == 1){
      float rgb_class[4];
      partclassdata *partclassi;
      char *device_ptr;
      char *prop_id;
      char prop_buffer[255];
      size_t len;

      partclassi = partclassinfo + npartclassinfo;
      partclassi->kind=PARTICLES;
      if(Match(buffer,"CLASS_OF_HUMANS") == 1)partclassi->kind=HUMANS;
      FGETS(buffer,255,stream);

      GetLabels(buffer,partclassi->kind,&device_ptr,&prop_id,prop_buffer);
      if(prop_id!=NULL){
        device_ptr=NULL;
      }
      partclassi->prop=NULL;

      partclassi->sphere=NULL;
      partclassi->smv_device=NULL;
      partclassi->device_name=NULL;
      if(device_ptr!=NULL){
        partclassi->sphere=GetSmvObjectType("SPHERE",missing_device);

        partclassi->smv_device=GetSmvObjectType(device_ptr,missing_device);
        if(partclassi->smv_device!=NULL){
          len = strlen(device_ptr);
          NewMemory((void **)&partclassi->device_name,len+1);
          STRCPY(partclassi->device_name,device_ptr);
        }
        else{
          char tube[10];

          strcpy(tube,"TUBE");
          len = strlen(tube);
          NewMemory((void **)&partclassi->device_name,len+1);
          STRCPY(partclassi->device_name,tube);
          partclassi->smv_device=GetSmvObjectType(tube,missing_device);
        }
      }

      TrimBack(buffer);
      len=strlen(buffer);
      partclassi->name=NULL;
      if(len>0){
        NewMemory((void **)&partclassi->name,len+1);
        STRCPY(partclassi->name,TrimFront(buffer));
      }

      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f",rgb_class,rgb_class+1,rgb_class+2);
      rgb_class[3]=1.0;
      partclassi->rgb=GetColorPtr(rgb_class);

      partclassi->ntypes=0;
      partclassi->xyz=NULL;
      partclassi->maxpoints=0;
      partclassi->labels=NULL;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&partclassi->ntypes);
      partclassi->ntypes+=2;
      partclassi->nvars_dep=partclassi->ntypes-2+3; // subtract off two "dummies" at beginning and add 3 at end for r,g,b
      if(partclassi->ntypes>0){
        flowlabels *labelj;
        char shortdefaultlabel[]="Uniform";
        char longdefaultlabel[]="Uniform color";
        int j;

        NewMemory((void **)&partclassi->labels,partclassi->ntypes*sizeof(flowlabels));

        labelj = partclassi->labels; // placeholder for hidden

        labelj->longlabel=NULL;
        labelj->shortlabel=NULL;
        labelj->unit=NULL;

        labelj = partclassi->labels+1;  // placeholder for default

        labelj->longlabel=NULL;
        NewMemory((void **)&labelj->longlabel,strlen(longdefaultlabel)+1);
        strcpy(labelj->longlabel,longdefaultlabel);
        labelj->shortlabel=NULL;
        NewMemory((void **)&labelj->shortlabel,strlen(shortdefaultlabel)+1);
        strcpy(labelj->shortlabel,shortdefaultlabel);
        labelj->unit=NULL;

        partclassi->col_azimuth=-1;
        partclassi->col_diameter=-1;
        partclassi->col_elevation=-1;
        partclassi->col_length=-1;
        partclassi->col_u_vel=-1;
        partclassi->col_v_vel=-1;
        partclassi->col_w_vel=-1;
        partclassi->vis_type=PART_POINTS;
        for(j=2;j<partclassi->ntypes;j++){
          labelj = partclassi->labels+j;
          labelj->longlabel=NULL;
          labelj->shortlabel=NULL;
          labelj->unit=NULL;
          ReadLabels(labelj,stream,NULL);
          partclassi->vars_dep[j-2]=labelj->shortlabel;
          if(strcmp(labelj->shortlabel,"DIAMETER")==0){
            partclassi->col_diameter=j-2;
          }
          if(strcmp(labelj->shortlabel,"LENGTH")==0){
            partclassi->col_length=j-2;
          }
          if(strcmp(labelj->shortlabel,"AZIMUTH")==0){
            partclassi->col_azimuth=j-2;
          }
          if(strcmp(labelj->shortlabel,"ELEVATION")==0){
            partclassi->col_elevation=j-2;
          }
          if(STRCMP(labelj->shortlabel,"U-VEL")==0){
            partclassi->col_u_vel=j-2;
          }
          if(STRCMP(labelj->shortlabel,"V-VEL")==0){
            partclassi->col_v_vel=j-2;
          }
          if(STRCMP(labelj->shortlabel,"W-VEL")==0){
            partclassi->col_w_vel=j-2;
          }
        }
      }
      partclassi->diameter=1.0;
      partclassi->length=1.0;
      partclassi->azimuth=0.0;
      partclassi->elevation=0.0;
      partclassi->dx=0.0;
      partclassi->dy=0.0;
      partclassi->dz=0.0;
      if(device_ptr!=NULL){
        float diameter, length, azimuth, elevation;

        FGETS(buffer,255,stream);
        sscanf(buffer,"%f %f %f %f",&diameter,&length,&azimuth,&elevation);
        partclassi->diameter=diameter;
        partclassi->length=length;
        partclassi->azimuth=azimuth;
        partclassi->elevation=elevation;
      }
      npartclassinfo++;
      continue;
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ LABEL ++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"LABEL") == 1){

      /*
      LABEL
      x y z r g b tstart tstop
      label

      */
      {
        float *xyz, *frgbtemp, *tstart_stop;
        int *rgbtemp;
        labeldata labeltemp, *labeli;

        labeli = &labeltemp;

        xyz = labeli->xyz;
        frgbtemp = labeli->frgb;
        rgbtemp = labeli->rgb;
        tstart_stop = labeli->tstart_stop;

        labeli->labeltype=TYPE_SMV;
        FGETS(buffer,255,stream);
        frgbtemp[0]=-1.0;
        frgbtemp[1]=-1.0;
        frgbtemp[2]=-1.0;
        frgbtemp[3]=1.0;
        tstart_stop[0]=-1.0;
        tstart_stop[1]=-1.0;
        sscanf(buffer,"%f %f %f %f %f %f %f %f",
          xyz,xyz+1,xyz+2,
          frgbtemp,frgbtemp+1,frgbtemp+2,
          tstart_stop,tstart_stop+1);

        if(frgbtemp[0]<0.0||frgbtemp[1]<0.0||frgbtemp[2]<0.0||frgbtemp[0]>1.0||frgbtemp[1]>1.0||frgbtemp[2]>1.0){
          labeli->useforegroundcolor=1;
        }
        else{
          labeli->useforegroundcolor=0;
        }
        FGETS(buffer,255,stream);
        TrimBack(buffer);
        bufferptr = TrimFront(buffer);

        strcpy(labeli->name,bufferptr);
        rgbtemp[0]=frgbtemp[0]*255;
        rgbtemp[1]=frgbtemp[1]*255;
        rgbtemp[2]=frgbtemp[2]*255;
        LabelInsert(labeli);
      }
      continue;
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ TICKS ++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"TICKS") == 1){
      ntickinfo++;
      ntickinfo_smv++;
      {
        tickdata *ticki;
        float *begt, *endt;
        int *nbarst;
        float term;
        float length=0.0;
        float *dxyz;
        float sum;

        ticki = tickinfo + ntickinfo - 1;
        begt = ticki->begin;
        endt = ticki->end;
        nbarst=&ticki->nbars;
        dxyz = ticki->dxyz;

        if(FGETS(buffer,255,stream)==NULL){
          BREAK;
        }
        *nbarst=0;
        sscanf(buffer,"%f %f %f %f %f %f %i",begt,begt+1,begt+2,endt,endt+1,endt+2,nbarst);
        if(*nbarst<1)*nbarst=1;
        if(FGETS(buffer,255,stream)==NULL){
          BREAK;
        }
        {
          float *rgbtemp;

          rgbtemp=ticki->rgb;
          rgbtemp[0]=-1.0;
          rgbtemp[1]=-1.0;
          rgbtemp[2]=-1.0;
          ticki->width=-1.0;
          sscanf(buffer,"%f %i %f %f %f %f",&ticki->dlength,&ticki->dir,rgbtemp,rgbtemp+1,rgbtemp+2,&ticki->width);
          if(rgbtemp[0]<0.0||rgbtemp[0]>1.0||
             rgbtemp[1]<0.0||rgbtemp[1]>1.0||
             rgbtemp[2]<0.0||rgbtemp[2]>1.0){
            ticki->useforegroundcolor=1;
          }
          else{
            ticki->useforegroundcolor=0;
          }
          if(ticki->width<0.0)ticki->width=1.0;
        }
        for(i=0;i<3;i++){
          term = endt[i]-begt[i];
          length += term*term;
        }
        if(length<=0.0){
          endt[0]=begt[0]+1.0;
          length = 1.0;
        }
        ticki->length=sqrt(length);
        dxyz[0] =  0.0;
        dxyz[1] =  0.0;
        dxyz[2] =  0.0;
        switch(ticki->dir){
        case XRIGHT:
        case XLEFT:
          dxyz[0]=1.0;
          break;
        case YFRONT:
        case YBACK:
          dxyz[1]=1.0;
          break;
        case ZBOTTOM:
        case ZTOP:
          dxyz[2]=1.0;
          break;
        default:
          ASSERT(FFALSE);
          break;
        }
        if(ticki->dir<0){
          for(i=0;i<3;i++){
            dxyz[i]=-dxyz[i];
          }
        }
        sum = 0.0;
        sum = dxyz[0]*dxyz[0] + dxyz[1]*dxyz[1] + dxyz[2]*dxyz[2];
        if(sum>0.0){
          sum=sqrt(sum);
          dxyz[0] *= (ticki->dlength/sum);
          dxyz[1] *= (ticki->dlength/sum);
          dxyz[2] *= (ticki->dlength/sum);
        }
      }
      continue;
    }


  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OUTLINE ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"OUTLINE") == 1){
      outlinedata *outlinei;

      noutlineinfo++;
      outlinei = outlineinfo + noutlineinfo - 1;
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      sscanf(buffer,"%i",&outlinei->nlines);
      if(outlinei->nlines>0){
        NewMemory((void **)&outlinei->x1,outlinei->nlines*sizeof(float));
        NewMemory((void **)&outlinei->y1,outlinei->nlines*sizeof(float));
        NewMemory((void **)&outlinei->z1,outlinei->nlines*sizeof(float));
        NewMemory((void **)&outlinei->x2,outlinei->nlines*sizeof(float));
        NewMemory((void **)&outlinei->y2,outlinei->nlines*sizeof(float));
        NewMemory((void **)&outlinei->z2,outlinei->nlines*sizeof(float));
        for(i=0;i<outlinei->nlines;i++){
          FGETS(buffer,255,stream);
          sscanf(buffer,"%f %f %f %f %f %f",
            outlinei->x1+i,outlinei->y1+i, outlinei->z1+i,
            outlinei->x2+i,outlinei->y2+i, outlinei->z2+i
            );
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ CADGEOM ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"CADGEOM") == 1){
      size_t len;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);
      cadgeominfo[ncadgeom].order=NULL;
      cadgeominfo[ncadgeom].quad=NULL;
      cadgeominfo[ncadgeom].file=NULL;
      if(FILE_EXISTS_CASEDIR(bufferptr)==YES){
        if(NewMemory((void **)&cadgeominfo[ncadgeom].file,(unsigned int)(len+1))==0)return 2;
        STRCPY(cadgeominfo[ncadgeom].file,bufferptr);
        ReadCADGeom(cadgeominfo+ncadgeom);
        ncadgeom++;
      }
      else{
        PRINTF(_("***Error: CAD geometry file: %s could not be opened"),bufferptr);
        PRINTF("\n");
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OFFSET ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OFFSET") == 1){
      ioffset++;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SURFDEF ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SURFDEF") == 1){
      FGETS(buffer,255,stream);
      bufferptr=TrimFrontBack(buffer);
      strcpy(surfacedefaultlabel,TrimFront(bufferptr));
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SMOKE3D ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(
      Match(buffer,"SMOKE3D") == 1||
      Match(buffer,"VSMOKE3D") == 1||
      Match(buffer,"SMOKF3D") == 1||
      Match(buffer,"VSMOKF3D") == 1
      ||Match(buffer, "SMOKG3D") == 1 ||
      Match(buffer, "VSMOKG3D") == 1
      ){

      size_t len;
      size_t lenbuffer;
      float temp_val=-1.0;
      char *buffer_temp;
      int filetype=C_GENERATED;
      int blocknumber;

      if(setup_only==1)continue;
      if(Match(buffer,"SMOKF3D") == 1||Match(buffer,"VSMOKF3D") == 1||
         Match(buffer, "SMOKG3D") == 1 || Match(buffer, "VSMOKG3D") == 1
        ){
        filetype=FORTRAN_GENERATED;
      }

      if(Match(buffer,"VSMOKE3D") == 1||Match(buffer,"VSMOKF3D") == 1|| Match(buffer, "VSMOKG3D") == 1){
        int idummy;

        buffer_temp=buffer+8;
        sscanf(buffer_temp,"%i %f",&idummy,&temp_val);
        if(temp_val>0.0)hrrpuv_max_smv=temp_val;
      }
      nn_smoke3d++;
      TrimBack(buffer);
      len=strlen(buffer);
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(len>8){
        char *buffer3;

        buffer3=buffer+8;
        sscanf(buffer3,"%i",&blocknumber);
        blocknumber--;
      }
      if(FGETS(buffer,255,stream)==NULL){
        nsmoke3dinfo--;
        BREAK;
      }
      bufferptr=TrimFrontBack(buffer);
      len=strlen(buffer);
      lenbuffer=len;
      {
        smoke3ddata *smoke3di;
        int ii;

        smoke3di = smoke3dinfo + ismoke3d;

#ifdef _DEBUG
        if(nsmoke3dinfo>500&&(ismoke3d%100==0||ismoke3d==nsmoke3dinfo-1)){
          PRINTF("     examining %i'st 3D smoke file\n",ismoke3dcount);
        }
#endif
        ismoke3dcount++;

        if(NewMemory((void **)&smoke3di->reg_file,(unsigned int)(len+1))==0)return 2;
        STRCPY(smoke3di->reg_file,bufferptr);

        smoke3di->ntimes = 0;
        smoke3di->ntimes_old = 0;
        smoke3di->filetype=filetype;
        smoke3di->is_zlib=0;
        smoke3di->seq_id=nn_smoke3d;
        smoke3di->autoload=0;
        smoke3di->compression_type=UNKNOWN;
        for(ii = 0;ii < MAXSMOKETYPES;ii++){
          smoke3di->smokestate[ii].color = NULL;
        }
        smoke3di->file=NULL;
        smoke3di->smokeframe_in=NULL;
        smoke3di->smokeframe_comp_list=NULL;
        smoke3di->smokeframe_out=NULL;
        smoke3di->timeslist=NULL;
        smoke3di->smoke_comp_all=NULL;
        smoke3di->smokeview_tmp=NULL;
        smoke3di->times=NULL;
        smoke3di->use_smokeframe=NULL;
        smoke3di->nchars_compressed_smoke=NULL;
        smoke3di->nchars_compressed_smoke_full=NULL;
        smoke3di->maxval = -1.0;
        smoke3di->frame_all_zeros=NULL;

        smoke3di->display=0;
        smoke3di->loaded=0;
        smoke3di->request_load = 0;
        smoke3di->primary_file=0;
        smoke3di->blocknumber=blocknumber;
        smoke3di->lastiframe=-999;
        for(ii = 0;ii < MAXSMOKETYPES;ii++){
          smoke3di->smokestate[ii].index = -1;
        }
        smoke3di->ismoke3d_time=0;

        STRCPY(buffer2,bufferptr);
        STRCAT(buffer2,".svz");

        len=lenbuffer+4;
        if(NewMemory((void **)&smoke3di->comp_file,(unsigned int)(len+1))==0)return 2;
        STRCPY(smoke3di->comp_file,buffer2);

        if(FILE_EXISTS_CASEDIR(smoke3di->comp_file)==YES){
          smoke3di->file=smoke3di->comp_file;
          smoke3di->is_zlib=1;
          smoke3di->compression_type = COMPRESSED_ZLIB;
        }
        else{
          smoke3di->file=smoke3di->reg_file;
        }
        if(FILE_EXISTS_CASEDIR(smoke3di->file)==YES){
          if(ReadLabels(&smoke3di->label,stream,NULL)==2)return 2;
          if(strcmp(smoke3di->label.longlabel,"HRRPUV")==0){
            show_hrrcutoff_active=1;
          }
          ismoke3d++;
        }
        else{
          if(ReadLabels(&smoke3di->label,stream,NULL)==2)return 2;
          nsmoke3dinfo--;
        }
        if(Match(smoke3di->label.shortlabel,"soot")==1|| Match(smoke3di->label.shortlabel, "rho_C") == 1){
          smoke3di->type=SOOT;
          nsmoke3d_soot++;
        }
        else if(Match(smoke3di->label.shortlabel,"hrrpuv")==1){
          smoke3di->type=HRRPUV;
          nsmoke3d_hrrpuv++;
        }
        else if(Match(smoke3di->label.shortlabel, "temp") == 1){
          smoke3di->type = TEMP;
          nsmoke3d_temp++;
        }
        else if(Match(smoke3di->label.shortlabel, "rho_CO2") == 1){
          smoke3di->type = CO2;
          nsmoke3d_co2++;
        }
        else{
          smoke3di->type=SOOT;
          nsmoke3d_soot++;
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SURFACE ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SURFACE") ==1){
      surfdata *surfi;
      float s_color[4];
      int s_type;
      float temp_ignition,emis,t_width, t_height;
      size_t len;
      char *buffer3;

      surfi = surfinfo + nsurfinfo;
      InitSurface(surfi);
      FGETS(buffer,255,stream);
      TrimBack(buffer);
      len=strlen(buffer);
      NewMemory((void **)&surfi->surfacelabel,(len+1)*sizeof(char));
      strcpy(surfi->surfacelabel,TrimFront(buffer));
      if(strstr(surfi->surfacelabel,"MIRROR")!=NULL||
         strstr(surfi->surfacelabel,"INTERPOLATED")!=NULL||
         strstr(surfi->surfacelabel,"OPEN")!=NULL){
        surfi->obst_surface=0;
      }
      if(strstr(surfi->surfacelabel,"INERT")!=NULL){
        surfinfo[0].obst_surface=1;
      }

      temp_ignition=TEMP_IGNITION_MAX;
      emis = 1.0;
      t_width=1.0;
      t_height=1.0;
      s_type=0;
      s_color[0]=surfi->color[0];
      s_color[1]=surfi->color[1];
      s_color[2]=surfi->color[2];
      //s_color[3]=1.0;
      s_color[3]=surfi->color[3];
      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f",&temp_ignition,&emis);
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %f %f %f %f %f %f",
        &s_type,&t_width, &t_height,s_color,s_color+1,s_color+2,s_color+3);

      surfi->type=s_type;
      if(s_color[0]<=0.0&&s_color[1]<=0.0&&s_color[2]<=0.0){
        if(s_color[0]!=0.0||s_color[1]!=0.0||s_color[2]!=0.0){
          surfi->invisible=1;
          surfi->type=BLOCK_hidden;
          s_color[0]=-s_color[0];
          s_color[1]=-s_color[1];
          s_color[2]=-s_color[2];
        }
      }
      surfi->color = GetColorPtr(s_color);
      if(s_color[3]<0.99){
        surfi->transparent=1;
      }
      surfi->transparent_level=1.0;
      surfi->temp_ignition=temp_ignition;
      surfi->emis=emis;
      surfi->t_height=t_height;
      surfi->t_width=t_width;
      surfi->texturefile=NULL;
      surfi->textureinfo=NULL;

      FGETS(buffer,255,stream);
      TrimBack(buffer);
      buffer3 = TrimFront(buffer);
      {
        int found_texture;
        char texturebuffer[1024];

        found_texture=0;
        if(texturedir!=NULL&&FILE_EXISTS_CASEDIR(buffer3)==NO){
          STRCPY(texturebuffer,texturedir);
          STRCAT(texturebuffer,dirseparator);
          STRCAT(texturebuffer,buffer3);
          if(FILE_EXISTS_CASEDIR(texturebuffer)==YES){
            if(NewMemory((void **)&surfi->texturefile,strlen(texturebuffer)+1)==0)return 2;
            STRCPY(surfi->texturefile,texturebuffer);
            found_texture=1;
          }
        }
        if(FILE_EXISTS_CASEDIR(buffer3)==YES){
          len=strlen(buffer3);
          if(NewMemory((void **)&surfi->texturefile,(unsigned int)(len+1))==0)return 2;
          STRCPY(surfi->texturefile,buffer3);
          found_texture=1;
        }
        if(buffer3!=NULL&&found_texture==0&&strncmp(buffer3,"null",4)!=0){
          fprintf(stderr,"*** Error: The texture file %s was not found\n",buffer3);
        }
      }
      nsurfinfo++;
      continue;
    }
  /*
    ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ MATERIAL  +++++++++++++++++++++++++
    ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"MATERIAL") ==1){
      matldata *matli;
      float s_color[4];
      int len;

      matli = matlinfo + nmatlinfo;
      InitMatl(matli);

      FGETS(buffer,255,stream);
      TrimBack(buffer);
      len=strlen(buffer);
      NewMemory((void **)&matli->matllabel,(len+1)*sizeof(char));
      strcpy(matli->matllabel,TrimFront(buffer));

      s_color[0]=matli->color[0];
      s_color[1]=matli->color[1];
      s_color[2]=matli->color[2];
      s_color[3]=matli->color[3];
      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f %f",s_color,s_color+1,s_color+2,s_color+3);

      s_color[0]=CLAMP(s_color[0],0.0,1.0);
      s_color[1]=CLAMP(s_color[1],0.0,1.0);
      s_color[2]=CLAMP(s_color[2],0.0,1.0);
      s_color[3]=CLAMP(s_color[3],0.0,1.0);

      matli->color = GetColorPtr(s_color);

      nmatlinfo++;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ GRID ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"GRID") == 1){
      meshdata *meshi;
      int mesh_type=0;
      float *xp, *yp, *zp;
      float *xp2, *yp2, *zp2;
      float *xplt_cen, *yplt_cen,*zplt_cen;

//      int lenbuffer;

//      TrimBack(buffer);
//      lenbuffer=strlen(buffer);
//      if(lenbuffer>4){
//        if(buffer[5]!=' ')continue;
//      }

      igrid++;
      if(meshinfo!=NULL){
        meshi=meshinfo+igrid-1;
        InitMesh(meshi);
        {
          size_t len_meshlabel;
          char *meshlabel;

          len_meshlabel=0;
          if(strlen(buffer)>5){
            meshlabel=TrimFront(buffer+5);
            TrimBack(meshlabel);
            len_meshlabel=strlen(meshlabel);
          }
          if(len_meshlabel>0){
            NewMemory((void **)&meshi->label,(len_meshlabel+1));
            strcpy(meshi->label,meshlabel);
          }
          else{
            sprintf(buffer,"%i",igrid);
            NewMemory((void **)&meshi->label,strlen(buffer)+1);
            strcpy(meshi->label,buffer);
          }
        }

      }
      setGRID=1;
      if(noGRIDpresent==1){
        ibartemp=2;
        jbartemp=2;
        kbartemp=2;
      }
      else{
        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %i %i %i",&ibartemp,&jbartemp,&kbartemp,&mesh_type);
      }
      if(ibartemp<1)ibartemp=1;
      if(jbartemp<1)jbartemp=1;
      if(kbartemp<1)kbartemp=1;
      xp=NULL; yp=NULL; zp=NULL;
      xp2=NULL; yp2=NULL; zp2=NULL;
      if(
         NewMemory((void **)&xp,sizeof(float)*(ibartemp+1))==0||
         NewMemory((void **)&yp,sizeof(float)*(jbartemp+1))==0||
         NewMemory((void **)&zp,sizeof(float)*(kbartemp+1))==0||
         NewMemory((void **)&xplt_cen,sizeof(float)*ibartemp)==0||
         NewMemory((void **)&yplt_cen,sizeof(float)*jbartemp)==0||
         NewMemory((void **)&zplt_cen,sizeof(float)*kbartemp)==0||
         NewMemory((void **)&xp2,sizeof(float)*(ibartemp+1))==0||
         NewMemory((void **)&yp2,sizeof(float)*(jbartemp+1))==0||
         NewMemory((void **)&zp2,sizeof(float)*(kbartemp+1))==0
         )return 2;
      if(meshinfo!=NULL){
        meshi->mesh_type=mesh_type;
        meshi->xplt=xp;
        meshi->yplt=yp;
        meshi->zplt=zp;
        meshi->xplt_cen=xplt_cen;
        meshi->yplt_cen=yplt_cen;
        meshi->zplt_cen=zplt_cen;
        meshi->xplt_orig=xp2;
        meshi->yplt_orig=yp2;
        meshi->zplt_orig=zp2;
        meshi->ibar=ibartemp;
        meshi->jbar=jbartemp;
        meshi->kbar=kbartemp;
        meshi->plotx=ibartemp/2;
        meshi->ploty=jbartemp/2;
        meshi->plotz=kbartemp/2;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ ZONE ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"ZONE") == 1){
      char *filename;
      zonedata *zonei;
      char buffer_csv[1000],*buffer_csvptr;
      char *period=NULL;
      size_t len;
      int n;

      zonei = zoneinfo + izone_local;
      if(FGETS(buffer,255,stream)==NULL){
        nzoneinfo--;
        BREAK;
      }
      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);
      zonei->loaded=0;
      zonei->display=0;

      buffer_csvptr=buffer_csv;
      strcpy(buffer_csv,bufferptr);
      filename= GetZoneFileName(buffer_csvptr);
      if(filename!=NULL)period=strrchr(filename,'.');
      if(filename!=NULL&&period!=NULL&&strcmp(period,".csv")==0){
        zonei->csv=1;
        zonecsv=1;
      }
      else{
        zonei->csv=0;
        zonecsv=0;
        filename= GetZoneFileName(bufferptr);
      }

      if(filename==NULL){
        int nn;

        for(nn=0;nn<4;nn++){
          if(ReadLabels(&zonei->label[nn],stream,NULL)==2){
            return 2;
          }
        }
        nzoneinfo--;
      }
      else{
        len=strlen(filename);
        NewMemory((void **)&zonei->file,(unsigned int)(len+1));
        STRCPY(zonei->file,filename);
        for(n=0;n<4;n++){
          if(ReadLabels(&zonei->label[n],stream,NULL)==2){
            return 2;
          }
        }
        izone_local++;
      }
      if(colorlabelzone!=NULL){
        for(n=0;n<MAXRGB;n++){
          FREEMEMORY(colorlabelzone[n]);
        }
        FREEMEMORY(colorlabelzone);
      }
      CheckMemory;
      NewMemory((void **)&colorlabelzone,MAXRGB*sizeof(char *));
      for(n=0;n<MAXRGB;n++){
        colorlabelzone[n]=NULL;
      }
      for(n=0;n<nrgb;n++){
        NewMemory((void **)&colorlabelzone[n],11);
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ ROOM ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"ROOM") == 1){
      roomdata *roomi;

      isZoneFireModel=1;
      visFrame=0;
      roomdefined=1;
      iroom++;
      roomi = roominfo + iroom - 1;
      roomi->valid=0;
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      sscanf(buffer,"%f %f %f",&roomi->dx,&roomi->dy,&roomi->dz);
      roomi->valid=1;
      if(FGETS(buffer,255,stream)==NULL){
        roomi->x0=0.0;
        roomi->y0=0.0;
        roomi->z0=0.0;
      }
      else{
        sscanf(buffer,"%f %f %f",&roomi->x0,&roomi->y0,&roomi->z0);
      }
      roomi->x1=roomi->x0+roomi->dx;
      roomi->y1=roomi->y0+roomi->dy;
      roomi->z1=roomi->z0+roomi->dz;

      if(setPDIM==0){
        if(roomi->x0<xbar0)xbar0=roomi->x0;
        if(roomi->y0<ybar0)ybar0=roomi->y0;
        if(roomi->z0<zbar0)zbar0=roomi->z0;
        if(roomi->x1>xbar)xbar=roomi->x1;
        if(roomi->y1>ybar)ybar=roomi->y1;
        if(roomi->z1>zbar)zbar=roomi->z1;
      }
      continue;
    }
  }

  STOP_TIMER(pass2_time);

/*
   ************************************************************************
   ************************ end of pass 2 *********************************
   ************************************************************************
 */

  START_TIMER(pass3_time);

  CheckMemory;
  ParseDatabase(database_filename);


  if(setGRID==0){
    meshdata *meshi;
    float *xp, *yp, *zp;
    float *xp2, *yp2, *zp2;
    float *xplt_cen, *yplt_cen, *zplt_cen;
    int  nn;

    xp=NULL; yp=NULL; zp=NULL;
    xp2=NULL; yp2=NULL; zp2=NULL;
    if(
       NewMemory((void **)&xp,sizeof(float)*(ibartemp+1))==0||
       NewMemory((void **)&yp,sizeof(float)*(jbartemp+1))==0||
       NewMemory((void **)&zp,sizeof(float)*(kbartemp+1))==0||
       NewMemory((void **)&xplt_cen,sizeof(float)*ibartemp)==0||
       NewMemory((void **)&yplt_cen,sizeof(float)*jbartemp)==0||
       NewMemory((void **)&zplt_cen,sizeof(float)*kbartemp)==0||
       NewMemory((void **)&xp2,sizeof(float)*(ibartemp+1))==0||
       NewMemory((void **)&yp2,sizeof(float)*(jbartemp+1))==0||
       NewMemory((void **)&zp2,sizeof(float)*(kbartemp+1))==0
       )return 2;
    for(nn=0;nn<=ibartemp;nn++){
      xp[nn]=xbar0+(float)nn*(xbar-xbar0)/(float)ibartemp;
    }
    for(nn=0;nn<=jbartemp;nn++){
      yp[nn]=ybar0+(float)nn*(ybar-ybar0)/(float)jbartemp;
    }
    for(nn=0;nn<=kbartemp;nn++){
      zp[nn]=zbar0+(float)nn*(zbar-zbar0)/(float)kbartemp;
    }
    meshi=meshinfo;
    InitMesh(meshi);
    meshi->xplt=xp;
    meshi->yplt=yp;
    meshi->zplt=zp;
    meshi->xplt_cen=xplt_cen;
    meshi->yplt_cen=yplt_cen;
    meshi->zplt_cen=zplt_cen;
    meshi->xplt_orig=xp2;
    meshi->yplt_orig=yp2;
    meshi->zplt_orig=zp2;
    meshi->ibar=ibartemp;
    meshi->jbar=jbartemp;
    meshi->kbar=kbartemp;
    meshi->plotx=ibartemp/2;
    meshi->ploty=jbartemp/2;
    meshi->plotz=kbartemp/2;
  }
  if(setPDIM==0&&roomdefined==1){
    meshdata *meshi;

    meshi=meshinfo;
    meshi->xyz_bar0[XXX]=xbar0;
    meshi->xyz_bar[XXX] =xbar;
    meshi->xcen=(xbar+xbar0)/2.0;
    meshi->xyz_bar0[YYY]=ybar0;
    meshi->xyz_bar[YYY] =ybar;
    meshi->ycen=(ybar+ybar0)/2.0;
    meshi->xyz_bar0[ZZZ]=zbar0;
    meshi->xyz_bar[ZZZ] =zbar;
    meshi->zcen=(zbar+zbar0)/2.0;
  }

  if(ndeviceinfo>0){
    if(NewMemory((void **)&deviceinfo,ndeviceinfo*sizeof(devicedata))==0)return 2;
    devicecopy=deviceinfo;;
  }
  ndeviceinfo=0;
#ifdef pp_READBUFFER
  REWIND(stream);
#else
  REWIND(stream1);
  if(stream2!=NULL)rewind(stream2);
  stream=stream1;
#endif
  PRINTF("%s","  pass 3\n");

  /*
   ************************************************************************
   ************************ start of pass 3 *******************************
   ************************************************************************
 */

  for(;;){
    if(FEOF(stream)!=0){
      BREAK;
    }
    if(FGETS(buffer,255,stream)==NULL){
      BREAK;
    }
    TrimBack(buffer);
    if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ AMBIENT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"AMBIENT")==1){
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      sscanf(buffer,"%f %f %f",&pref,&pamb,&tamb);
      continue;
    }
    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++ CUTCELLS ++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"CUTCELLS") == 1){
      meshdata *meshi;
      int imesh,ncutcells;

      sscanf(buffer+10,"%i",&imesh);
      imesh=CLAMP(imesh-1,0,nmeshes-1);
      meshi = meshinfo + imesh;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&ncutcells);
      meshi->ncutcells=ncutcells;

      if(ncutcells>0){
        NewMemory((void **)&meshi->cutcells,ncutcells*sizeof(int));
        for(i=0;i<1+(ncutcells-1)/15;i++){
          int cc[15],j;

          FGETS(buffer,255,stream);
          for(j=0;j<15;j++){
            cc[j]=0;
          }
          sscanf(buffer,"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
            cc,cc+1,cc+2,cc+3,cc+4,cc+5,cc+6,cc+7,cc+8,cc+9,cc+10,cc+11,cc+12,cc+13,cc+14);
          for(j=15*i;j<MIN(15*(i+1),ncutcells);j++){
            meshi->cutcells[j]=cc[j%15];
          }
        }
      }

      continue;
    }

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ DEVICE +++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    DEVICE
    label
    x y z xn yn zn state nparams ntextures
    p0 p1 ... p5
    p6 ...    p11
    texturefile1
    ...
    texturefilen

    */
    if(
      (Match(buffer,"DEVICE") == 1)&&
      (Match(buffer,"DEVICE_ACT") != 1)
      ){
      devicedata *devicei;

      devicei = deviceinfo + ndeviceinfo;
      ParseDevicekeyword(stream,devicei);
      CheckMemory;
      ndeviceinfo++;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ THCP ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"THCP") == 1){
      meshdata *meshi;
      float normdenom;
      char *device_label;
      int tempval;

      if(ioffset==0)ioffset=1;
      meshi=meshinfo + ioffset - 1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&tempval);
      if(tempval<0)tempval=0;
      meshi->ntc=tempval;
      ntc_total += meshi->ntc;
      hasSensorNorm=0;
      if(meshi->ntc>0){
        int nn;

        for(nn=0;nn<meshi->ntc;nn++){
          float *xyz, *xyznorm;
          FGETS(buffer,255,stream);

          strcpy(devicecopy->label,"");
          xyz = devicecopy->xyz;
          xyznorm = devicecopy->xyznorm;
          xyz[0]=0.0;
          xyz[1]=0.0;
          xyz[2]=0.0;
          xyznorm[0]=0.0;
          xyznorm[1]=0.0;
          xyznorm[2]=-1.0;
          device_label= GetDeviceLabel(buffer);
          sscanf(buffer,"%f %f %f %f %f %f",xyz,xyz+1,xyz+2,xyznorm,xyznorm+1,xyznorm+2);
          normdenom=0.0;
          normdenom+=xyznorm[0]*xyznorm[0];
          normdenom+=xyznorm[1]*xyznorm[1];
          normdenom+=xyznorm[2]*xyznorm[2];
          if(normdenom>0.1){
            hasSensorNorm=1;
            normdenom=sqrt(normdenom);
            xyznorm[0]/=normdenom;
            xyznorm[1]/=normdenom;
            xyznorm[2]/=normdenom;
          }
          if(device_label==NULL){
            if(isZoneFireModel==1){
              devicecopy->object = GetSmvObjectType("target",thcp_object_backup);
            }
            else{
              devicecopy->object = GetSmvObjectType("thermoc4",thcp_object_backup);
            }
          }
          else{
            devicecopy->object = GetSmvObjectType(device_label,thcp_object_backup);
          }
          GetElevAz(xyznorm,&devicecopy->dtheta,devicecopy->rotate_axis,NULL);

          InitDevice(devicecopy,xyz,0,NULL,NULL,xyznorm,0,0,NULL,"target");
          devicecopy->prop=NULL;

          devicecopy++;
          ndeviceinfo++;
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SPRK ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SPRK") == 1){
      meshdata *meshi;
      char *device_label;
      int tempval;

      meshi=meshinfo + ioffset - 1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&tempval);
      if(tempval<0)tempval=0;
      meshi->nspr=tempval;
      nspr_total += meshi->nspr;
      if(meshi->nspr>0){
        float *xsprcopy, *ysprcopy, *zsprcopy;
        int nn;

        FREEMEMORY(meshi->xspr); FREEMEMORY(meshi->yspr); FREEMEMORY(meshi->zspr); FREEMEMORY(meshi->tspr);
        if(NewMemory((void **)&meshi->xspr,meshi->nspr*sizeof(float))==0||
           NewMemory((void **)&meshi->yspr,meshi->nspr*sizeof(float))==0||
           NewMemory((void **)&meshi->zspr,meshi->nspr*sizeof(float))==0||
           NewMemory((void **)&meshi->tspr,meshi->nspr*sizeof(float))==0||
           NewMemory((void **)&meshi->xsprplot,meshi->nspr*sizeof(float))==0||
           NewMemory((void **)&meshi->ysprplot,meshi->nspr*sizeof(float))==0||
           NewMemory((void **)&meshi->zsprplot,meshi->nspr*sizeof(float))==0)return 2;
        for(nn=0;nn<meshi->nspr;nn++){
          meshi->tspr[nn]=99999.;
        }
        xsprcopy=meshi->xspr;
        ysprcopy=meshi->yspr;
        zsprcopy=meshi->zspr;
        for(nn=0;nn<meshi->nspr;nn++){
          float *xyznorm;
          float normdenom;

          FGETS(buffer,255,stream);
          xyznorm = devicecopy->xyznorm;
          xyznorm[0]=0.0;
          xyznorm[1]=0.0;
          xyznorm[2]=-1.0;
          device_label= GetDeviceLabel(buffer);
          sscanf(buffer,"%f %f %f %f %f %f",xsprcopy,ysprcopy,zsprcopy,xyznorm,xyznorm+1,xyznorm+2);
          devicecopy->act_time=-1.0;
          devicecopy->type = DEVICE_SPRK;
          devicecopy->xyz[0]=*xsprcopy;
          devicecopy->xyz[1]=*ysprcopy;
          devicecopy->xyz[2]=*zsprcopy;
          normdenom=0.0;
          normdenom+=xyznorm[0]*xyznorm[0];
          normdenom+=xyznorm[1]*xyznorm[1];
          normdenom+=xyznorm[2]*xyznorm[2];
          normdenom=sqrt(normdenom);
          if(normdenom>0.001){
            xyznorm[0]/=normdenom;
            xyznorm[1]/=normdenom;
            xyznorm[2]/=normdenom;
          }
          if(device_label==NULL){
            devicecopy->object = GetSmvObjectType("sprinkler_upright",sprinkler_upright_object_backup);
          }
          else{
            devicecopy->object = GetSmvObjectType(device_label,sprinkler_upright_object_backup);
          }
          GetElevAz(xyznorm,&devicecopy->dtheta,devicecopy->rotate_axis,NULL);

          InitDevice(devicecopy,NULL,0,NULL,NULL,xyznorm,0,0,NULL,NULL);

          devicecopy++;
          ndeviceinfo++;

          xsprcopy++; ysprcopy++; zsprcopy++;
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ HEAT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"HEAT") == 1){
      meshdata *meshi;
      char *device_label;
      int tempval;
      int  nn;

      meshi=meshinfo + ioffset - 1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&tempval);
      if(tempval<0)tempval=0;
      meshi->nheat=tempval;
      nheat_total += meshi->nheat;
      if(meshi->nheat>0){
        float *xheatcopy, *yheatcopy, *zheatcopy;

        FREEMEMORY(meshi->xheat); FREEMEMORY(meshi->yheat); FREEMEMORY(meshi->zheat); FREEMEMORY(meshi->theat);
        FREEMEMORY(meshi->xheatplot); FREEMEMORY(meshi->yheatplot); FREEMEMORY(meshi->zheatplot);
        if(NewMemory((void **)&meshi->xheat,meshi->nheat*sizeof(float))==0||
           NewMemory((void **)&meshi->yheat,meshi->nheat*sizeof(float))==0||
           NewMemory((void **)&meshi->zheat,meshi->nheat*sizeof(float))==0||
           NewMemory((void **)&meshi->theat,meshi->nheat*sizeof(float))==0||
           NewMemory((void **)&meshi->xheatplot,meshi->nheat*sizeof(float))==0||
           NewMemory((void **)&meshi->yheatplot,meshi->nheat*sizeof(float))==0||
           NewMemory((void **)&meshi->zheatplot,meshi->nheat*sizeof(float))==0)return 2;
        for(nn=0;nn<meshi->nheat;nn++){
          meshi->theat[nn]=99999.;
        }
        xheatcopy=meshi->xheat;
        yheatcopy=meshi->yheat;
        zheatcopy=meshi->zheat;
        for(nn=0;nn<meshi->nheat;nn++){
          float *xyznorm;
          float normdenom;
          FGETS(buffer,255,stream);
          xyznorm=devicecopy->xyznorm;
          xyznorm[0]=0.0;
          xyznorm[1]=0.0;
          xyznorm[2]=-1.0;
          device_label= GetDeviceLabel(buffer);
          sscanf(buffer,"%f %f %f %f %f %f",xheatcopy,yheatcopy,zheatcopy,xyznorm,xyznorm+1,xyznorm+2);
          devicecopy->type = DEVICE_HEAT;
          devicecopy->act_time=-1.0;
          devicecopy->xyz[0]=*xheatcopy;
          devicecopy->xyz[1]=*yheatcopy;
          devicecopy->xyz[2]=*zheatcopy;
          normdenom=0.0;
          normdenom+=xyznorm[0]*xyznorm[0];
          normdenom+=xyznorm[1]*xyznorm[1];
          normdenom+=xyznorm[2]*xyznorm[2];
          normdenom=sqrt(normdenom);
          if(normdenom>0.001){
            xyznorm[0]/=normdenom;
            xyznorm[1]/=normdenom;
            xyznorm[2]/=normdenom;
          }
          if(device_label==NULL){
            devicecopy->object = GetSmvObjectType("heat_detector",heat_detector_object_backup);
          }
          else{
            devicecopy->object = GetSmvObjectType(device_label,heat_detector_object_backup);
          }
          GetElevAz(xyznorm,&devicecopy->dtheta,devicecopy->rotate_axis,NULL);

          InitDevice(devicecopy,NULL,0,NULL,NULL,xyznorm,0,0,NULL,NULL);
          devicecopy->prop=NULL;

          devicecopy++;
          ndeviceinfo++;
          xheatcopy++; yheatcopy++; zheatcopy++;

        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SMOD ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SMOD") == 1){
      float xyz[3];
      int sdnum;
      char *device_label;
      int nn;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&sdnum);
      if(sdnum<0)sdnum=0;
      for(nn=0;nn<sdnum;nn++){
        float *xyznorm;
        float normdenom;

        xyznorm = devicecopy->xyznorm;
        xyznorm[0]=0.0;
        xyznorm[1]=0.0;
        xyznorm[2]=-1.0;
        device_label= GetDeviceLabel(buffer);
        FGETS(buffer,255,stream);
        sscanf(buffer,"%f %f %f %f %f %f",xyz,xyz+1,xyz+2,xyznorm,xyznorm+1,xyznorm+2);
        devicecopy->type = DEVICE_SMOKE;
        devicecopy->act_time=-1.0;
        devicecopy->xyz[0]=xyz[0];
        devicecopy->xyz[1]=xyz[1];
        devicecopy->xyz[2]=xyz[2];
        normdenom=0.0;
        normdenom+=xyznorm[0]*xyznorm[0];
        normdenom+=xyznorm[1]*xyznorm[1];
        normdenom+=xyznorm[2]*xyznorm[2];
        normdenom=sqrt(normdenom);
        if(normdenom>0.001){
          xyznorm[0]/=normdenom;
          xyznorm[1]/=normdenom;
          xyznorm[2]/=normdenom;
        }
        if(device_label==NULL){
          devicecopy->object = GetSmvObjectType("smoke_detector",smoke_detector_object_backup);
        }
        else{
          devicecopy->object = GetSmvObjectType(device_label,smoke_detector_object_backup);
        }
        GetElevAz(xyznorm,&devicecopy->dtheta,devicecopy->rotate_axis,NULL);

        InitDevice(devicecopy,xyz,0,NULL,NULL,xyznorm,0,0,NULL,NULL);

        devicecopy++;
        ndeviceinfo++;

      }
      continue;
    }
  }

  STOP_TIMER(pass3_time);

  /*
   ************************************************************************
   ************************ end of pass 3 *********************************
   ************************************************************************
 */

  START_TIMER(pass4_time);

  // look for DEVICE entries in "experimental" spread sheet files

  if(ncsvinfo>0){
    int *nexp_devices=NULL;
    devicedata *devicecopy2;

    NewMemory((void **)&nexp_devices,ncsvinfo*sizeof(int));
    for(i=0;i<ncsvinfo;i++){
      csvdata *csvi;

      csvi = csvinfo + i;
      if(csvi->type==CSVTYPE_EXT){
        nexp_devices[i] = GetNDevices(csvi->file);
        ndeviceinfo_exp += nexp_devices[i];
      }
    }
    if(ndeviceinfo>0){
      if(ndeviceinfo_exp>0){
        ResizeMemory((void **)&deviceinfo,(ndeviceinfo_exp+ndeviceinfo)*sizeof(devicedata));
      }
    }
    else{
      if(ndeviceinfo_exp>0)NewMemory((void **)&deviceinfo,ndeviceinfo_exp*sizeof(devicedata));
    }
    devicecopy2 = deviceinfo+ndeviceinfo;
    ndeviceinfo+=ndeviceinfo_exp;

    for(i=0;i<ncsvinfo;i++){
      csvdata *csvi;

      csvi = csvinfo + i;
      if(csvi->type==CSVTYPE_EXT){
        ReadDeviceHeader(csvi->file,devicecopy2,nexp_devices[i]);
        devicecopy2 += nexp_devices[i];
      }
    }
    FREEMEMORY(nexp_devices);
    devicecopy2=deviceinfo;
  }

  // define texture data structures by constructing a list of unique file names from surfinfo and devices

  UpdateDeviceTextures();
  if(nsurfinfo>0||ndevice_texture_list>0){
    if(NewMemory((void **)&textureinfo,(nsurfinfo+ndevice_texture_list)*sizeof(texturedata))==0)return 2;
  }
  InitTextures();

/*
    Initialize blockage labels and blockage surface labels

    Define default surface for each block.
    Define default vent surface for each block.

  */

  surfacedefault=&sdefault;
  for(i=0;i<nsurfinfo;i++){
    if(strcmp(surfacedefaultlabel,surfinfo[i].surfacelabel)==0){
      surfacedefault=surfinfo+i;
      break;
    }
  }
  vent_surfacedefault=&v_surfacedefault;
  for(i=0;i<nsurfinfo;i++){
    if(strcmp(vent_surfacedefault->surfacelabel,surfinfo[i].surfacelabel)==0){
      vent_surfacedefault=surfinfo+i;
      break;
    }
  }

  exterior_surfacedefault=&e_surfacedefault;
  for(i=0;i<nsurfinfo;i++){
    if(strcmp(exterior_surfacedefault->surfacelabel,surfinfo[i].surfacelabel)==0){
      exterior_surfacedefault=surfinfo+i;
      break;
    }
  }

  nvents=0;
  itrnx=0, itrny=0, itrnz=0, igrid=0, ipdim=0, iobst=0, ivent=0, icvent=0;
  ioffset=0;
  npartclassinfo=0;
  if(noffset==0)ioffset=1;

/*
   ************************************************************************
   ************************ start of pass 4 *******************************
   ************************************************************************
 */

#ifdef pp_READBUFFER
  REWIND(stream);
#else
  REWIND(stream1);
  if(stream2!=NULL)rewind(stream2);
  stream=stream1;
#endif
  PRINTF("%s","  pass 4\n");
  startpass=1;
  CheckMemory;

  for(;;){
    if(FEOF(stream)!=0){
      BREAK;
    }

    if(nVENT==0){
      nVENT=0;
      strcpy(buffer,"VENT");
    }
    else{
      if(startpass==1&&noGRIDpresent==1){
        strcpy(buffer,"GRID");
        startpass=0;
      }
      else{
        if(FGETS(buffer,255,stream)==NULL){
          BREAK;
        }
        if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;
      }
    }
    CheckMemory;
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++ CLASS_OF_PARTICLES +++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"CLASS_OF_PARTICLES") == 1||
       Match(buffer,"CLASS_OF_HUMANS") == 1){
      partclassdata *partclassi;
      char *device_ptr;
      char *prop_id;
      char prop_buffer[255];

      partclassi = partclassinfo + npartclassinfo;
      partclassi->kind=PARTICLES;
      if(Match(buffer,"CLASS_OF_HUMANS") == 1)partclassi->kind=HUMANS;
      FGETS(buffer,255,stream);

      GetLabels(buffer,partclassi->kind,&device_ptr,&prop_id,prop_buffer);
      partclassi->prop=GetPropID(prop_id);
      UpdatePartClassDepend(partclassi);

      npartclassinfo++;
      continue;
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ HRRPUVCUT ++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"HRRPUVCUT") == 1){
      int nhrrpuvcut;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&nhrrpuvcut);
      if(nhrrpuvcut>=1){
        FGETS(buffer,255,stream);
        sscanf(buffer,"%f",&global_hrrpuv_cutoff);
        load_hrrpuv_cutoff = global_hrrpuv_cutoff;
        for(i=1;i<nhrrpuvcut;i++){
          FGETS(buffer,255,stream);
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OFFSET ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OFFSET") == 1){
      meshdata *meshi;

      ioffset++;
      meshi=meshinfo+ioffset-1;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f",meshi->offset,meshi->offset+1,meshi->offset+2);
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++ HFLOWGEOM/VHFLOWGEOM/MFLOWGEOM +++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    have_zonevents=0;
    if(nzventsnew==0&&(
       Match(buffer, "VENTGEOM")==1||
       Match(buffer, "HFLOWGEOM")==1||
       Match(buffer, "VFLOWGEOM")==1||
       Match(buffer, "MFLOWGEOM")==1))have_zonevents=1;
    if(nzventsnew>0&&(
       Match(buffer, "HVENTGEOM")==1||
       Match(buffer, "VVENTGEOM")==1||
       Match(buffer, "MVENTGEOM")==1))have_zonevents=1;
    if(nzventsnew > 0 && (
      Match(buffer, "HVENTPOS") == 1 ||
      Match(buffer, "VVENTPOS") == 1 ||
      Match(buffer, "MVENTPOS") == 1))have_zonevents = 2;
    if(have_zonevents==1){
      int vent_type=HFLOW_VENT;
      int vertical_vent_type=0;
      zventdata *zvi;
      float vent_area;
      int roomfrom, roomto, wall;
      roomdata *roomi;
      float color[4];
      float vent_width,ventoffset,bottom,top;

      nzvents++;
      zvi = zventinfo + nzvents - 1;
      if(Match(buffer,"VFLOWGEOM")==1||Match(buffer,"VVENTGEOM")==1)vent_type=VFLOW_VENT;
      if(Match(buffer, "MFLOWGEOM") == 1 || Match(buffer, "MVENTGEOM") == 1)vent_type = MFLOW_VENT;
      zvi->vent_type=vent_type;
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      color[0]=1.0;
      color[1]=0.0;
      color[2]=1.0;
      color[3]=1.0;
      if(vent_type==HFLOW_VENT){
        float area_fraction=1.0;

        nzhvents++;
        sscanf(buffer,"%i %i %i %f %f %f %f %f %f %f %f",
          &roomfrom,&roomto, &wall,&vent_width,&ventoffset,&bottom,&top,
          color,color+1,color+2,&area_fraction
          );

        zvi->area=vent_width*(top-bottom);

        if(roomfrom<1||roomfrom>nrooms)roomfrom=nrooms+1;
        roomi = roominfo + roomfrom-1;
        zvi->room1 = roomi;

        if(roomto<1||roomto>nrooms)roomto=nrooms+1;
        zvi->room2=roominfo+roomto-1;

        zvi->wall=wall;
        zvi->z0=roomi->z0+bottom;
        zvi->z1=roomi->z0+top;
        switch(wall){
        case FRONT_WALL:
          zvi->x0 = roomi->x0 + ventoffset;
          zvi->x1 = zvi->x0 + vent_width;
          zvi->y0 = roomi->y0;
          zvi->y1 = roomi->y0;
          break;
        case RIGHT_WALL:
          zvi->x0 = roomi->x1;
          zvi->x1 = roomi->x1;
          zvi->y0 = roomi->y0 + ventoffset;
          zvi->y1 = zvi->y0 + vent_width;
          break;
        case BACK_WALL:
          zvi->x0 = roomi->x0 + ventoffset;
          zvi->x1 = zvi->x0 + vent_width;
          zvi->y0 = roomi->y1;
          zvi->y1 = roomi->y1;
          break;
        case LEFT_WALL:
          zvi->x0 = roomi->x0;
          zvi->x1 = roomi->x0;
          zvi->y0 = roomi->y0 + ventoffset;
          zvi->y1 = zvi->y0 + vent_width;
          break;
        default:
          ASSERT(FFALSE);
        }
        zvi->color = GetColorPtr(color);
        zvi->area_fraction = area_fraction;
      }
      else if(vent_type==VFLOW_VENT){
        float ventside;
        float xcen, ycen;
        int r_from, r_to;
        float area_fraction=1.0;

        nzvvents++;
        sscanf(buffer,"%i %i %i %f %i %f %f %f %f",
          &r_from,&r_to,&wall,&vent_area,&vertical_vent_type,
          color,color+1,color+2, &area_fraction
          );
        zvi->wall=wall;
        roomfrom=r_from;
        roomto=r_to;
        if(roomfrom<1||roomfrom>nrooms){
          roomfrom=r_to;
          roomto=r_from;
          if(roomfrom<1||roomfrom>nrooms){
            roomfrom=1;
          }
        }
        roomi = roominfo + roomfrom - 1;
        vent_area=ABS(vent_area);
        ventside=sqrt(vent_area);
        xcen = (roomi->x0+roomi->x1)/2.0;
        ycen = (roomi->y0+roomi->y1)/2.0;
        if(wall==BOTTOM_WALL){
          zvi->z0 = roomi->z0;
          zvi->z1 = roomi->z0;
        }
        else{
          zvi->z0 = roomi->z1;
          zvi->z1 = roomi->z1;
        }
        switch(vertical_vent_type){
        case ZONEVENT_SQUARE:
        case ZONEVENT_CIRCLE:
          zvi->xcen = xcen;
          zvi->ycen = ycen;
          zvi->x0 = xcen - ventside/2.0;
          zvi->x1 = xcen + ventside/2.0;
          zvi->y0 = ycen - ventside/2.0;
          zvi->y1 = ycen + ventside/2.0;
          break;
        default:
          ASSERT(FFALSE);
          break;
        }

        zvi->vertical_vent_type = vertical_vent_type;
        zvi->area = vent_area;
        zvi->area_fraction = area_fraction;
        zvi->color = GetColorPtr(color);
      }
      else if(vent_type==MFLOW_VENT){
        nzmvents++;
        ReadZVentData(zvi, buffer, ZVENT_1ROOM);
      }
      CheckMemory;
      continue;
    }
    if(have_zonevents==2){
      int vent_type = HFLOW_VENT;
      zventdata *zvi;

      if(Match(buffer, "VFLOWPOS") == 1 || Match(buffer, "VVENTPOS") == 1)vent_type = VFLOW_VENT;
      if(Match(buffer, "MFLOWPOS") == 1 || Match(buffer, "MVENTPOS") == 1)vent_type = MFLOW_VENT;

      nzvents++;
      zvi = zventinfo + nzvents - 1;

      zvi->vent_type = vent_type;
      if(FGETS(buffer, 255, stream) == NULL){
        BREAK;
      }

      CheckMemory;
      switch(vent_type){
      case HFLOW_VENT:
        nzhvents++;
        ReadZVentData(zvi, buffer,ZVENT_2ROOM);
        break;
      case VFLOW_VENT:
        nzvvents++;
        ReadZVentData(zvi, buffer, ZVENT_2ROOM);
        break;
      case MFLOW_VENT:
        nzmvents++;
        ReadZVentData(zvi, buffer, ZVENT_1ROOM);
        break;
      default:
        ASSERT(FFALSE);
        break;
      }
      CheckMemory;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ FIRE ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"FIRE")==1){
      firedata *firei;
      int roomnumber;

      ifire++;
      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      firei = fireinfo + ifire - 1;
      sscanf(buffer,"%i %f %f %f",&roomnumber,&firei->x,&firei->y,&firei->z);
      if(roomnumber>=1&&roomnumber<=nrooms){
        roomdata *roomi;

        roomi = roominfo + roomnumber - 1;
        firei->valid=1;
        firei->roomnumber=roomnumber;
        firei->absx=roomi->x0+firei->x;
        firei->absy=roomi->y0+firei->y;
        firei->absz=roomi->z0+firei->z;
      }
      else{
        firei->valid=0;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ PDIM ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"PDIM") == 1){
      meshdata *meshi;
      float *meshrgb;

      ipdim++;
      meshi=meshinfo+ipdim-1;
      meshrgb = meshi->meshrgb;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f %f %f %f %f %f %f",&xbar0,&xbar,&ybar0,&ybar,&zbar0,&zbar,meshrgb,meshrgb+1,meshrgb+2);

      if(meshrgb[0]!=0.0||meshrgb[1]!=0.0||meshrgb[2]!=0.0){
        meshi->meshrgb_ptr=meshi->meshrgb;
      }
      else{
        meshi->meshrgb_ptr=NULL;
      }

      meshi->xyz_bar0[XXX]=xbar0;
      meshi->xyz_bar[XXX] =xbar;
      meshi->xcen=(xbar+xbar0)/2.0;
      meshi->xyz_bar0[YYY]=ybar0;
      meshi->xyz_bar[YYY] =ybar;
      meshi->ycen =(ybar+ybar0)/2.0;
      meshi->xyz_bar0[ZZZ]=zbar0;
      meshi->xyz_bar[ZZZ] =zbar;
      meshi->zcen =(zbar+zbar0)/2.0;
      InitBoxClipInfo(&(meshi->box_clipinfo),xbar0,xbar,ybar0,ybar,zbar0,zbar);
      if(ntrnx==0){
        int nn;

        for(nn = 0; nn<=meshi->ibar; nn++){
          meshi->xplt[nn] = xbar0+(float)nn*(xbar-xbar0)/(float)meshi->ibar;
        }
      }
      if(ntrny==0){
        int nn;

        for(nn = 0; nn<=meshi->jbar; nn++){
          meshi->yplt[nn] = ybar0+(float)nn*(ybar-ybar0)/(float)meshi->jbar;
        }
      }
      if(ntrnz==0){
        int nn;

        for(nn = 0; nn<=meshi->kbar; nn++){
          meshi->zplt[nn]=zbar0+(float)nn*(zbar-zbar0)/(float)meshi->kbar;
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ TRNX ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"TRNX")==1){
      float *xpltcopy;
      int idummy;
      int nn;

      itrnx++;
      xpltcopy=meshinfo[itrnx-1].xplt;
      ibartemp=meshinfo[itrnx-1].ibar;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i ",&idummy);
      for(nn=0;nn<idummy;nn++){
        FGETS(buffer,255,stream);
      }
      for(nn=0;nn<=ibartemp;nn++){
        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %f",&idummy,xpltcopy);xpltcopy++;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ TRNY ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"TRNY") == 1){
      float *ypltcopy;
      int idummy;
      int nn;

      itrny++;
      ypltcopy=meshinfo[itrny-1].yplt;
      jbartemp=meshinfo[itrny-1].jbar;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i ",&idummy);
      for(nn=0;nn<idummy;nn++){
        FGETS(buffer,255,stream);
      }
      for(nn=0;nn<=jbartemp;nn++){
//        if(jbartemp==2&&nn==2)continue;
        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %f",&idummy,ypltcopy);
        ypltcopy++;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ TRNZ ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"TRNZ") == 1){
      float *zpltcopy;
      int idummy;
      int nn;

      itrnz++;
      zpltcopy=meshinfo[itrnz-1].zplt;
      kbartemp=meshinfo[itrnz-1].kbar;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i ",&idummy);
      for(nn=0;nn<idummy;nn++){
        FGETS(buffer,255,stream);
      }
      for(nn=0;nn<=kbartemp;nn++){
        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %f",&idummy,zpltcopy);zpltcopy++;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ TREE ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    /*
typedef struct {
  float xyz[3];
  float trunk_diam;
  float tree_height;
  float base_diam;
  float base_height;
*/
    if(Match(buffer,"TREE") == 1){
      FGETS(buffer,255,stream);
      if(ntreeinfo!=0)continue;
      sscanf(buffer,"%i",&ntreeinfo);
      if(ntreeinfo>0){
        NewMemory((void **)&treeinfo,sizeof(treedata)*ntreeinfo);
        for(i=0;i<ntreeinfo;i++){
          treedata *treei;
          float *xyz;

          treei = treeinfo + i;
          treei->time_char=-1.0;
          treei->time_complete=-1.0;
          xyz = treei->xyz;
          FGETS(buffer,255,stream);
          sscanf(buffer,"%f %f %f %f %f %f %f",
            xyz,xyz+1,xyz+2,
            &treei->trunk_diam,&treei->tree_height,
            &treei->base_diam,&treei->base_height);
        }
      }
      continue;
    }
    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ TREESTATE ++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    */
    if(Match(buffer, "TREESTATE")==1){
      int tree_index, tree_state;
      float tree_time;
      treedata *treei;

      if(ntreeinfo==0)continue;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i %i %f",&tree_index,&tree_state,&tree_time);
      if(tree_index>=1&&tree_index<=ntreeinfo){
        treei = treeinfo + tree_index - 1;
        if(tree_state==1){
          treei->time_char = tree_time;
        }
        else if(tree_state==2){
          treei->time_complete = tree_time;
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OBST ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"OBST") == 1){
      meshdata *meshi;
      propdata *prop;
      char *proplabel;
      int n_blocks=0;
      int n_blocks_normal=0;
      unsigned char *is_block_terrain=NULL;
      int iblock;
      int nn;
      size_t len;

      CheckMemoryOff;
      iobst++;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&n_blocks);

      meshi=meshinfo+iobst-1;
      if(n_blocks<=0)n_blocks=0;
      meshi->nbptrs=n_blocks;
      n_blocks_normal=n_blocks;
      if(n_blocks==0)continue;

      if(autoterrain==1){
        is_block_terrain=meshi->is_block_terrain;
        n_blocks_normal=0;
        for(iblock=0;iblock<n_blocks;iblock++){
          if(is_block_terrain[iblock]==0)n_blocks_normal++;
        }
        meshi->nbptrs=n_blocks_normal;
      }
      meshi->blockageinfoptrs=NULL;
      if(n_blocks_normal>0){
        NewMemory((void **)&meshi->blockageinfoptrs,sizeof(blockagedata *)*n_blocks_normal);
      }

      ntotal_blockages+=n_blocks_normal;
      nn=-1;
      for(iblock=0;iblock<n_blocks;iblock++){
        int s_num[6];
        blockagedata *bc;

        if(autoterrain==1&&meshi->is_block_terrain!=NULL&&meshi->is_block_terrain[iblock]==1){
          FGETS(buffer,255,stream);
          continue;
        }
        nn++;
        meshi->blockageinfoptrs[nn]=NULL;
        NewMemory((void **)&meshi->blockageinfoptrs[nn],sizeof(blockagedata));
        bc=meshi->blockageinfoptrs[nn];
        InitObst(bc,surfacedefault,nn+1,iobst-1);
        FGETS(buffer,255,stream);
        TrimBack(buffer);
        for(i=0;i<6;i++){
          s_num[i]=-1;
        }
        proplabel=strchr(buffer,'%');
        prop=NULL;
        if(proplabel!=NULL){
          proplabel++;
          TrimBack(proplabel);
          proplabel = TrimFront(proplabel);
          for(i=0;i<npropinfo;i++){
            propdata *propi;

            propi = propinfo + i;
            if(STRCMP(proplabel,propi->label)==0){
              prop = propi;
              propi->inblockage=1;
              break;
            }
          }
        }
        bc->prop=prop;
        {
          float t_origin[3];
          float *xyzEXACT;

          t_origin[0]=texture_origin[0];
          t_origin[1]=texture_origin[1];
          t_origin[2]=texture_origin[2];
          xyzEXACT = bc->xyzEXACT;
          sscanf(buffer,"%f %f %f %f %f %f %i %i %i %i %i %i %i %f %f %f",
            xyzEXACT,xyzEXACT+1,xyzEXACT+2,xyzEXACT+3,xyzEXACT+4,xyzEXACT+5,
            &(bc->blockage_id),s_num+DOWN_X,s_num+UP_X,s_num+DOWN_Y,s_num+UP_Y,s_num+DOWN_Z,s_num+UP_Z,
            t_origin,t_origin+1,t_origin+2);
          bc->xmin=xyzEXACT[0];
          bc->xmax=xyzEXACT[1];
          bc->ymin=xyzEXACT[2];
          bc->ymax=xyzEXACT[3];
          bc->zmin=xyzEXACT[4];
          bc->zmax=xyzEXACT[5];
          bc->texture_origin[0]=t_origin[0];
          bc->texture_origin[1]=t_origin[1];
          bc->texture_origin[2]=t_origin[2];
          if(bc->blockage_id<0){
            bc->changed=1;
            bc->blockage_id=-bc->blockage_id;
          }
        }

        /* define block label */

        sprintf(buffer,"**blockage %i",bc->blockage_id);
        len=strlen(buffer);
        ResizeMemory((void **)&bc->label,(len+1)*sizeof(char));
        strcpy(bc->label,buffer);

        for(i=0;i<6;i++){
          surfdata *surfi;

          if(surfinfo==NULL||s_num[i]<0||s_num[i]>=nsurfinfo)continue;
          surfi=surfinfo+s_num[i];
          bc->surf[i]=surfi;
        }
        for(i=0;i<6;i++){
          bc->surf[i]->used_by_obst=1;
        }
        SetSurfaceIndex(bc);
      }

      nn=-1;
      for(iblock=0;iblock<n_blocks;iblock++){
        blockagedata *bc;
        int *ijk;
        int colorindex, blocktype;

        if(autoterrain==1&&meshi->is_block_terrain!=NULL&&meshi->is_block_terrain[iblock]==1){
          FGETS(buffer,255,stream);
          continue;
        }
        nn++;

        bc=meshi->blockageinfoptrs[nn];
        colorindex=-1;
        blocktype=-1;

        /*
        OBST format:
        i1 i2 j1 j2 k1 k2 colorindex blocktype r g b : ignore rgb if blocktype != -3

        int colorindex, blocktype;
        colorindex: -1 default color
                    -2 invisible
                    -3 use r g b color
                    >=0 color/color2/texture index
        blocktype: 0 regular block
                   2 outline
                   3 smoothed block
                   (note: if blocktype&8 == 8 then this is a "terrain" blockage
                         if so then subtract 8 and set bc->is_wuiblock=1)
        r g b           colors used if colorindex==-3
        */

        FGETS(buffer,255,stream);
        ijk = bc->ijk;
        sscanf(buffer,"%i %i %i %i %i %i %i %i",
          ijk,ijk+1,ijk+2,ijk+3,ijk+4,ijk+5,
          &colorindex,&blocktype);
        if((blocktype&3)==3)blocktype -= 3; // convert any smooth blocks to 'normal' blocks
        if(blocktype>0&&(blocktype&8)==8){
          bc->is_wuiblock=1;
          blocktype -= 8;
        }

        /* custom color */

        if(colorindex==0||colorindex==7)colorindex=-3;

        if(colorindex==-3){
          float s_color[4];

          ijk = bc->ijk;

          s_color[0]=-1.0;
          s_color[1]=-1.0;
          s_color[2]=-1.0;
          s_color[3]=1.0;

          sscanf(buffer,"%i %i %i %i %i %i %i %i %f %f %f %f",
            ijk,ijk+1,ijk+2,ijk+3,ijk+4,ijk+5,
            &colorindex,&blocktype,s_color,s_color+1,s_color+2,s_color+3);
          if(blocktype>0&&(blocktype&8)==8){
            bc->is_wuiblock=1;
            blocktype -= 8;
          }
          if(s_color[3]<0.999){
            bc->transparent=1;
          }
          if(colorindex==0||colorindex==7){
            switch(colorindex){
            case 0:
              s_color[0]=1.0;
              s_color[1]=1.0;
              s_color[2]=1.0;
              break;
            case 7:
              s_color[0]=0.0;
              s_color[1]=0.0;
              s_color[2]=0.0;
              break;
            default:
              ASSERT(FFALSE);
              break;
            }
            colorindex=-3;
          }
          if(s_color[0]>=0.0&&s_color[1]>=0.0&&s_color[2]>=0.0){
            bc->color=GetColorPtr(s_color);
          }
          bc->nnodes=(ijk[1]+1-ijk[0])*(ijk[3]+1-ijk[2])*(ijk[5]+1-ijk[4]);
          bc->useblockcolor=1;
        }
        else{
          if(colorindex>=0){
            bc->color = GetColorPtr(rgb[nrgb+colorindex]);
            bc->useblockcolor=1;
            bc->usecolorindex=1;
            bc->colorindex=colorindex;
            updateindexcolors=1;
          }
        }

        bc->dup=0;
        bc->colorindex=colorindex;
        bc->type=blocktype;

        if(colorindex==COLOR_INVISIBLE){
          bc->type=BLOCK_hidden;
//          bc->del=1;
          bc->invisible=1;
        }
        if(bc->useblockcolor==0){
          bc->color=bc->surf[0]->color;
        }
        else{
          if(colorindex==-1){
            updateindexcolors=1;
          }
        }
      }
      CheckMemoryOn;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ CVENT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
        /*
        CVENT
        ncvents
        xmin xmax ymin ymax zmin zmax id surface_index tx ty tz % x0 y0 z0 radius
          ....  (ncvents rows)
          ....
        imin imax jmin jmax kmin kmax ventindex venttype r g b
          ....
          ....

          ventindex: -99 or 99 : use default color
                     +n or -n : use n'th palette color
                     < 0       : DO NOT draw boundary file over this vent
                     > 0       : DO draw boundary file over this vent
          vent type: 0 solid surface
                     2 outline
                     -2 hidden
          r g b:     red, green, blue color components
                     only specify if you wish to over-ride surface or default
                     range from 0.0 to 1.0
        */
    if(Match(buffer,"CVENT") == 1){
      cventdata *cvinfo;
      meshdata *meshi;
      float *origin;
      int ncv;
      int j;

      icvent++;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&ncv);

      meshi = meshinfo + icvent - 1;
      meshi->cventinfo=NULL;
      meshi->ncvents=ncv;
      if(ncv==0)continue;
      ncvents+=ncv;

      NewMemory((void **)&cvinfo,ncv*sizeof(cventdata));
      meshi->cventinfo=cvinfo;

      for(j=0;j<ncv;j++){
        cventdata *cvi;
        char *cbuf;
        int s_num[1];
        float t_origin[3];

        cvi = meshi->cventinfo + j;
        cvi->isOpenvent=0;
        cvi->surf[0]=vent_surfacedefault;
        cvi->textureinfo[0]=NULL;
        cvi->texture_origin[0]=texture_origin[0];
        cvi->texture_origin[1]=texture_origin[1];
        cvi->texture_origin[2]=texture_origin[2];
        cvi->useventcolor=1;
        cvi->hideboundary=0;
        cvi->cvent_id=-1;
        cvi->color=NULL;
        cvi->blank=NULL;
        cvi->showtime = NULL;
        cvi->showhide = NULL;
        cvi->showtimelist = NULL;

        origin=cvi->origin;
        s_num[0]=-1;
        t_origin[0]=texture_origin[0];
        t_origin[1]=texture_origin[1];
        t_origin[2]=texture_origin[2];
        FGETS(buffer,255,stream);
        sscanf(buffer,"%f %f %f %f %f %f %i %i %f %f %f",
          &cvi->xmin,&cvi->xmax,&cvi->ymin,&cvi->ymax,&cvi->zmin,&cvi->zmax,
          &cvi->cvent_id,s_num,t_origin,t_origin+1,t_origin+2);

        if(surfinfo!=NULL&&s_num[0]>=0&&s_num[0]<nsurfinfo){
          cvi->surf[0]=surfinfo+s_num[0];
          if(cvi->surf[0]!=NULL&&strncmp(cvi->surf[0]->surfacelabel,"OPEN",4)==0){
            cvi->isOpenvent=1;
          }
          cvi->surf[0]->used_by_vent=1;
        }
        if(t_origin[0]<=-998.0){
          t_origin[0]=texture_origin[0];
          t_origin[1]=texture_origin[1];
          t_origin[2]=texture_origin[2];
        }
        cvi->texture_origin[0]=t_origin[0];
        cvi->texture_origin[1]=t_origin[1];
        cvi->texture_origin[2]=t_origin[2];
        origin[0]=0.0;
        origin[1]=0.0;
        origin[2]=0.0;
        cvi->radius=0.0;
        cbuf=strchr(buffer,'%');
        if(cbuf!=NULL){
          TrimBack(cbuf);
          cbuf++;
          cbuf=TrimFront(cbuf);
          if(strlen(cbuf)>0){
            sscanf(cbuf,"%f %f %f %f",origin,origin+1,origin+2,&cvi->radius);
          }
        }
      }
      for(j=0;j<ncv;j++){
        cventdata *cvi;
        float *vcolor;
        int venttype,ventindex;
        float s_color[4],s2_color[4];

        s2_color[0]=-1.0;
        s2_color[1]=-1.0;
        s2_color[2]=-1.0;
        s2_color[3]=1.0;

        cvi = meshi->cventinfo + j;

        // use properties from &SURF

        cvi->type=cvi->surf[0]->type;
        vcolor=cvi->surf[0]->color;
        cvi->color=vcolor;

        s_color[0]=vcolor[0];
        s_color[1]=vcolor[1];
        s_color[2]=vcolor[2];
        s_color[3]=vcolor[3];
        venttype=-99;

        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %i %i %i %i %i %i %i %f %f %f",
          &cvi->imin,&cvi->imax,&cvi->jmin,&cvi->jmax,&cvi->kmin,&cvi->kmax,
          &ventindex,&venttype,s2_color,s2_color+1,s2_color+2);

        // use color from &VENT

        if(s2_color[0]>=0.0&&s2_color[1]>=0.0&&s2_color[2]>=0.0){
          s_color[0]=s2_color[0];
          s_color[1]=s2_color[1];
          s_color[2]=s2_color[2];
          cvi->useventcolor=1;
        }
        if(ventindex<0)cvi->hideboundary=1;
        if(venttype!=-99)cvi->type=venttype;

        // use pallet color

        if(ABS(ventindex)!=99){
          ventindex=ABS(ventindex);
          if(ventindex>nrgb2-1)ventindex=nrgb2-1;
          s_color[0]=rgb[nrgb+ventindex][0];
          s_color[1]=rgb[nrgb+ventindex][1];
          s_color[2]=rgb[nrgb+ventindex][2];
          s_color[3]=1.0;
          cvi->useventcolor=1;
        }
        s_color[3]=1.0; // set color to opaque until CVENT transparency is implemented
        cvi->color = GetColorPtr(s_color);
      }
      continue;
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ VENT ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

        /*
        new VENT format:

        xmin xmax ymin ymax zmin zmax ventid surf_index tx ty tz

        xyz min/max - vent boundaries
        ventid - index of vent
        surf_index - index of SURF
        tx, ty, yz - texture origin

        i1 i2 j1 j2 k1 k2 ventindex venttype r g b a

        ventindex: -99 or 99 : use default color
                   +n or -n : use n'th palette color
                   < 0       : DO NOT draw boundary file over this vent
                   > 0       : DO draw boundary file over this vent
        vent type: 0 solid surface
                   2 outline
                  -2 hidden
        r g b:     red, green, blue color components
                   only specify if you wish to over-ride surface or default
                   range from 0.0 to 1.0
        */

    if(Match(buffer,"VENT") == 1){
      meshdata *meshi;
      ventdata *vinfo;
      float *xplttemp,*yplttemp,*zplttemp;
      int nn;

      ivent++;
      meshi=meshinfo+ivent-1;
      xplttemp=meshi->xplt;
      yplttemp=meshi->yplt;
      zplttemp=meshi->zplt;
      if(nVENT==0){
        strcpy(buffer,"0 0");
        nVENT=1;
      }
      else{
        FGETS(buffer,255,stream);
      }
      ndummyvents=0;
      sscanf(buffer,"%i %i",&nvents,&ndummyvents);
      if(ndummyvents!=0){
        visFloor=0;
        visCeiling=0;
        visWalls=0;
      }
      meshi->nvents=nvents;
      meshi->ndummyvents=ndummyvents;
      vinfo=NULL;
      meshi->ventinfo=vinfo;
      if(NewMemory((void **)&vinfo,(nvents+12)*sizeof(ventdata))==0)return 2;
      meshi->ventinfo=vinfo;

      for(nn=0;nn<nvents+12;nn++){
        int s_num[6];
        ventdata *vi;

        vi=vinfo+nn;
        vi->type=VENT_SOLID;
        vi->dummyptr=NULL;
        vi->transparent=0;
        vi->useventcolor=0;
        vi->usecolorindex=0;
        vi->nshowtime=0;
        vi->isOpenvent=0;
        vi->hideboundary=0;
        vi->surf[0]=vent_surfacedefault;
        vi->textureinfo[0]=NULL;
        vi->texture_origin[0]=texture_origin[0];
        vi->texture_origin[1]=texture_origin[1];
        vi->texture_origin[2]=texture_origin[2];
        vi->colorindex=-1;
        if(nn>nvents-ndummyvents-1&&nn<nvents){
          vi->dummy=1;
        }
        else{
          vi->dummy=0;
        }
        s_num[0]=-1;
        if(nn<nvents){
          float t_origin[3];

          t_origin[0]=texture_origin[0];
          t_origin[1]=texture_origin[1];
          t_origin[2]=texture_origin[2];
          FGETS(buffer,255,stream);
          sscanf(buffer,"%f %f %f %f %f %f %i %i %f %f %f",
                 &vi->xmin,&vi->xmax,&vi->ymin,&vi->ymax,&vi->zmin,&vi->zmax,
                 &vi->vent_id,s_num,t_origin,t_origin+1,t_origin+2);
          if(t_origin[0]<=-998.0){
            t_origin[0]=texture_origin[0];
            t_origin[1]=texture_origin[1];
            t_origin[2]=texture_origin[2];
          }
          vi->texture_origin[0]=t_origin[0];
          vi->texture_origin[1]=t_origin[1];
          vi->texture_origin[2]=t_origin[2];
        }
        else{
          vi->xmin=meshi->xyz_bar0[XXX]+meshi->offset[XXX];
          vi->xmax=meshi->xyz_bar[XXX] +meshi->offset[XXX];
          vi->ymin=meshi->xyz_bar0[YYY]+meshi->offset[YYY];
          vi->ymax=meshi->xyz_bar[YYY] +meshi->offset[YYY];
          vi->zmin=meshi->xyz_bar0[ZZZ]+meshi->offset[ZZZ];
          vi->zmax=meshi->xyz_bar[ZZZ] +meshi->offset[ZZZ];
          s_num[0]=-1;
          switch(nn-nvents){
          case DOWN_Y:
          case DOWN_Y+6:
            vi->ymax=vi->ymin;
            break;
          case UP_X:
          case UP_X+6:
            vi->xmin=vi->xmax;
            break;
          case UP_Y:
          case UP_Y+6:
            vi->ymin=vi->ymax;
            break;
          case DOWN_X:
          case DOWN_X+6:
            vi->xmax=vi->xmin;
            break;
          case DOWN_Z:
          case DOWN_Z+6:
            vi->zmax=vi->zmin;
            break;
          case UP_Z:
          case UP_Z+6:
            vi->zmin=vi->zmax;
            break;
          default:
            ASSERT(FFALSE);
            break;
          }
          if(nn>=nvents+6){
            vi->surf[0]=exterior_surfacedefault;
          }
        }
        if(surfinfo!=NULL&&s_num[0]>=0&&s_num[0]<nsurfinfo){
          vi->surf[0]=surfinfo+s_num[0];
          if(vi->surf[0]!=NULL&&strncmp(vi->surf[0]->surfacelabel,"OPEN",4)==0){
            vi->isOpenvent=1;
          }
          vi->surf[0]->used_by_vent=1;
        }
        vi->color_bak=surfinfo[0].color;
      }
      for(nn=0;nn<nvents+12;nn++){
        ventdata *vi;
        int iv1, iv2, jv1, jv2, kv1, kv2;
        float s_color[4];
        int venttype,ventindex;

        vi = vinfo+nn;
        vi->type=vi->surf[0]->type;
        vi->color=vi->surf[0]->color;
        s_color[0]=vi->surf[0]->color[0];
        s_color[1]=vi->surf[0]->color[1];
        s_color[2]=vi->surf[0]->color[2];
        s_color[3]=vi->surf[0]->color[3];
        venttype=-99;
        if(nn<nvents){
          float s2_color[4];

          s2_color[0]=-1.0;
          s2_color[1]=-1.0;
          s2_color[2]=-1.0;
          s2_color[3]=1.0;

          FGETS(buffer,255,stream);
          sscanf(buffer,"%i %i %i %i %i %i %i %i %f %f %f %f",
               &iv1,&iv2,&jv1,&jv2,&kv1,&kv2,
               &ventindex,&venttype,
               s2_color,s2_color+1,s2_color+2,s2_color+3);
          if(s2_color[0]>=0.0&&s2_color[1]>=0.0&&s2_color[2]>=0.0){
            s_color[0]=s2_color[0];
            s_color[1]=s2_color[1];
            s_color[2]=s2_color[2];

            if(s2_color[3]<0.99){
              vi->transparent=1;
              s_color[3]=s2_color[3];
            }
            vi->useventcolor=1;
            ventindex = SIGN(ventindex)*99;
          }
          if(ventindex<0)vi->hideboundary=1;
          if(venttype!=-99)vi->type=venttype;
          if(ABS(ventindex)!=99){
            ventindex=ABS(ventindex);
            if(ventindex>nrgb2-1)ventindex=nrgb2-1;
            s_color[0]=rgb[nrgb+ventindex][0];
            s_color[1]=rgb[nrgb+ventindex][1];
            s_color[2]=rgb[nrgb+ventindex][2];
            s_color[3]=1.0;
            vi->colorindex=ventindex;
            vi->usecolorindex=1;
            vi->useventcolor=1;
            updateindexcolors=1;
          }
          vi->color = GetColorPtr(s_color);
        }
        else{
          iv1=0;
          iv2 = meshi->ibar;
          jv1=0;
          jv2 = meshi->jbar;
          kv1=0;
          kv2 = meshi->kbar;
          ventindex=-99;
          vi->dir=nn-nvents;
          if(vi->dir>5)vi->dir-=6;
          vi->dir2=0;
          switch(nn-nvents){
          case DOWN_Y:
          case DOWN_Y+6:
            jv2=jv1;
            if(nn>=nvents+6)vi->dir=UP_Y;
            vi->dir2=YDIR;
            break;
          case UP_X:
          case UP_X+6:
            iv1=iv2;
            if(nn>=nvents+6)vi->dir=DOWN_X;
            vi->dir2=XDIR;
            break;
          case UP_Y:
          case UP_Y+6:
            jv1=jv2;
            if(nn>=nvents+6)vi->dir=DOWN_Y;
            vi->dir2=YDIR;
            break;
          case DOWN_X:
          case DOWN_X+6:
            iv2=iv1;
            if(nn>=nvents+6)vi->dir=UP_X;
            vi->dir2=XDIR;
            break;
          case DOWN_Z:
          case DOWN_Z+6:
            kv2=kv1;
            if(nn>=nvents+6)vi->dir=UP_Z;
            vi->dir2=ZDIR;
            break;
          case UP_Z:
          case UP_Z+6:
            kv1=kv2;
            if(nn>=nvents+6)vi->dir=DOWN_Z;
            vi->dir2=ZDIR;
            break;
          default:
            ASSERT(FFALSE);
            break;
          }
        }
        if(vi->transparent==1)nvent_transparent++;
        vi->linewidth=&ventlinewidth;
        vi->showhide=NULL;
        vi->showtime=NULL;
        vi->showtimelist=NULL;
        vi->xvent1 = xplttemp[iv1];
        vi->xvent2 = xplttemp[iv2];
        vi->yvent1 = yplttemp[jv1];
        vi->yvent2 = yplttemp[jv2];
        vi->zvent1 = zplttemp[kv1];
        vi->zvent2 = zplttemp[kv2];
        vi->xvent1_orig = xplttemp[iv1];
        vi->xvent2_orig = xplttemp[iv2];
        vi->yvent1_orig = yplttemp[jv1];
        vi->yvent2_orig = yplttemp[jv2];
        vi->zvent1_orig = zplttemp[kv1];
        vi->zvent2_orig = zplttemp[kv2];
        vi->imin = iv1;
        vi->imax = iv2;
        vi->jmin = jv1;
        vi->jmax = jv2;
        vi->kmin = kv1;
        vi->kmax = kv2;
        if(nn>=nvents&&nn<nvents+6){
          vi->color=foregroundcolor;
        }
        ASSERT(vi->color!=NULL);
      }
      for(nn=0;nn<nvents-ndummyvents;nn++){
        int j;
        ventdata *vi;

        vi = meshi->ventinfo + nn;
        for(j=nvents-ndummyvents;j<nvents;j++){ // look for dummy vent that matches real vent
          ventdata *vj;

          vj = meshi->ventinfo + j;
          if(vi->imin!=vj->imin&&vi->imax!=vj->imax)continue;
          if(vi->jmin!=vj->jmin&&vi->jmax!=vj->jmax)continue;
          if(vi->kmin!=vj->kmin&&vi->kmax!=vj->kmax)continue;
          vi->dummyptr=vj;
          vj->dummyptr=vi;
        }
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ PART ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"PRT5")==1||Match(buffer,"EVA5")==1
      ){
      unsigned int lenkey;
      partdata *parti;
      int blocknumber;
      size_t len;
      char *buffer3;

      if(setup_only == 1)continue;
      nn_part++;

      parti = partinfo + ipart;

      lenkey=4;
      parti->evac=0;
      if(Match(buffer,"EVA5")==1
        ){
        parti->evac=1;
        nevac++;
      }
      len=strlen(buffer);
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(len>lenkey+1){
        buffer3=buffer+lenkey;
        if(parti->evac==1){
          float zoffset=0.0;

          sscanf(buffer3,"%i %f",&blocknumber,&zoffset);
          parti->zoffset=zoffset;
        }
        else{
          sscanf(buffer3,"%i",&blocknumber);
        }
        blocknumber--;
      }

      parti->blocknumber=blocknumber;
      parti->seq_id=nn_part;
      parti->autoload=0;
#ifdef pp_PARTDEFER
      parti->compute_bounds_color = 1;
#endif
      if(FGETS(buffer,255,stream)==NULL){
        npartinfo--;
        BREAK;
      }

      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);
      parti->reg_file=NULL;
      if(NewMemory((void **)&parti->reg_file,(unsigned int)(len+1))==0)return 2;
      STRCPY(parti->reg_file,bufferptr);

      parti->size_file=NULL;
      if(NewMemory((void **)&parti->size_file,(unsigned int)(len+1+3))==0)return 2;
      STRCPY(parti->size_file,bufferptr);
      STRCAT(parti->size_file,".sz");

      parti->hist_file = NULL;
      if(NewMemory((void **)&parti->hist_file, (unsigned int)(len + 1 + 5)) == 0)return 2;
      STRCPY(parti->hist_file, bufferptr);
      STRCAT(parti->hist_file, ".hist");

      // parti->size_file can't be written to, then put it in a world writable temp directory

      if(FILE_EXISTS_CASEDIR(parti->size_file)==NO&&curdir_writable ==NO&&smokeviewtempdir!=NULL){
        len = strlen(smokeviewtempdir)+strlen(bufferptr)+1+3+1;
        FREEMEMORY(parti->size_file);
        if(NewMemory((void **)&parti->size_file,(unsigned int)len)==0)return 2;
        STRCPY(parti->size_file,smokeviewtempdir);
        STRCAT(parti->size_file,dirseparator);
        STRCAT(parti->size_file,bufferptr);
        STRCAT(parti->size_file,".sz");
      }

      // parti->hist_file can't be written to, then put it in a world writable temp directory

      if(FILE_EXISTS_CASEDIR(parti->hist_file) == NO && curdir_writable == NO && smokeviewtempdir != NULL){
        len = strlen(smokeviewtempdir) + strlen(bufferptr) + 1 + 5 + 1;
        FREEMEMORY(parti->hist_file);
        if(NewMemory((void **)&parti->hist_file, (unsigned int)len) == 0)return 2;
        STRCPY(parti->hist_file, smokeviewtempdir);
        STRCAT(parti->hist_file, dirseparator);
        STRCAT(parti->hist_file, bufferptr);
        STRCAT(parti->hist_file, ".hist");
      }

      parti->comp_file = NULL;
      if(NewMemory((void **)&parti->comp_file,(unsigned int)(len+1+4))==0)return 2;
      STRCPY(parti->comp_file,bufferptr);
      STRCAT(parti->comp_file,".svz");

      if(FILE_EXISTS_CASEDIR(parti->comp_file)==YES){
        parti->compression_type=COMPRESSED_ZLIB;
        parti->file=parti->comp_file;
      }
      else{
        parti->compression_type=UNCOMPRESSED;
        if(FILE_EXISTS_CASEDIR(parti->reg_file)==YES){
          parti->file=parti->reg_file;
        }
        else{
          FREEMEMORY(parti->reg_file);
          FREEMEMORY(parti->comp_file);
          FREEMEMORY(parti->size_file);
          parti->file=NULL;
        }
      }
      parti->compression_type=UNCOMPRESSED;
      parti->sort_tags_loaded=0;
      parti->loaded=0;
      parti->display=0;
      parti->times=NULL;
      parti->timeslist=NULL;
      parti->histograms = NULL;
      parti->valmin = NULL;
      parti->valmax = NULL;

      parti->data5=NULL;
      parti->partclassptr=NULL;

      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&parti->nclasses);
      if(parti->nclasses>0){
        if(parti->file!=NULL)NewMemory((void **)&parti->partclassptr,parti->nclasses*sizeof(partclassdata *));
        for(i=0;i<parti->nclasses;i++){
          int iclass;
          int ic,iii;

          FGETS(buffer,255,stream);
          if(parti->file==NULL)continue;
          sscanf(buffer,"%i",&iclass);
          if(iclass<1)iclass=1;
          if(iclass>npartclassinfo)iclass=npartclassinfo;
          ic=0;
          for(iii=0;iii<npartclassinfo;iii++){
            partclassdata *pci;

            pci = partclassinfo + iii;
            if(parti->evac==1&&pci->kind!=HUMANS)continue;
            if(parti->evac==0&&pci->kind!=PARTICLES)continue;
            if(iclass-1==ic){
              parti->partclassptr[i]=pci;
              break;
            }
            ic++;
          }
        }
      }

      // if no classes were specified for the prt5 entry then assign it the default class

      if(parti->file!=NULL&&parti->nclasses==0){
        NewMemory((void **)&parti->partclassptr,sizeof(partclassdata *));
          parti->partclassptr[i]=partclassinfo + parti->nclasses;
      }
      if(fast_startup==1||(parti->file!=NULL&&FILE_EXISTS_CASEDIR(parti->file)==YES)){
        ipart++;
      }
      else{
        npartinfo--;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SYST ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"SYST") == 1){
      size_t len;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      len=strlen(buffer);
      buffer[len-1]='\0';
      STRCPY(LESsystem,buffer);
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ ENDIAN ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"ENDIAN") == 1){
      size_t len;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      len=strlen(buffer);
      buffer[len-1]='\0';
      strncpy(LESendian,buffer,1);
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ ENDF ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    /* ENDF supercedes ENDIAN */
    if(Match(buffer,"ENDF") == 1){
      FILE *ENDIANfile;
      size_t len;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);
      NewMemory((void **)&endian_filename,(unsigned int)(len+1));
      strcpy(endian_filename,bufferptr);
      ENDIANfile = fopen(endian_filename,"rb");
      if(ENDIANfile!=NULL){
        endian_native = GetEndian();
        FSEEK(ENDIANfile,4,SEEK_SET);
        fread(&endian_data,4,1,ENDIANfile);
        fclose(ENDIANfile);
        endian_smv=endian_native;
        if(endian_data!=1)endian_smv=1-endian_native;
        setendian=1;
      }
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ CHID +++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer,"CHID") == 1){
      size_t len;

      if(FGETS(buffer,255,stream)==NULL){
        BREAK;
      }
      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);
      FREEMEMORY(chidfilebase);
      NewMemory((void **)&chidfilebase,(unsigned int)(len+1));
      STRCPY(chidfilebase,bufferptr);

      if(chidfilebase!=NULL){
        NewMemory((void **)&hrr_csv_filename,(unsigned int)(strlen(chidfilebase)+8+1));
        STRCPY(hrr_csv_filename,chidfilebase);
        STRCAT(hrr_csv_filename,"_hrr.csv");
        if(FILE_EXISTS_CASEDIR(hrr_csv_filename)==NO){
          FREEMEMORY(hrr_csv_filename);
        }
      }
      continue;
    }

  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ SLCF ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if( (Match(buffer,"SLCF") == 1)||
        (Match(buffer,"SLCC") == 1)||
        (Match(buffer, "SLCD") == 1) ||
        (Match(buffer, "SLFL") == 1) ||
        (Match(buffer,"SLCT") == 1)
      ){
      char *slicelabelptr, slicelabel[256], *sliceparms, *sliceoffsetptr;
      float above_ground_level=0.0;
      float sliceoffset_fds=0.0;
      int terrain=0, cellcenter=0, facecenter=0, fire_line=0;
      int has_reg, has_comp;
      int ii1 = -1, ii2 = -1, jj1 = -1, jj2 = -1, kk1 = -1, kk2 = -1;
      int blocknumber;
      slicedata *sd;
      size_t len;
      int read_slice_header=0;

      if(setup_only == 1)continue;

      sliceoffsetptr = strchr(buffer, '$');
      if(sliceoffsetptr!=NULL){
        *sliceoffsetptr = 0;
        sliceoffsetptr++;
        sscanf(sliceoffsetptr, "%f", &sliceoffset_fds);
      }

      sliceparms=strchr(buffer,'&');
      if(sliceparms==NULL){
        read_slice_header=1;
      }
      else{
        sliceparms++;
        sliceparms[-1]=0;
        sscanf(sliceparms,"%i %i %i %i %i %i",&ii1,&ii2,&jj1,&jj2,&kk1,&kk2);
      }

      nn_slice++;
      slicelabelptr=strchr(buffer,'%');
      if(slicelabelptr!=NULL){
        *slicelabelptr=0;
        slicelabelptr++;
        TrimBack(slicelabelptr);
        slicelabelptr=TrimFront(slicelabelptr);
        strcpy(slicelabel,slicelabelptr);
        slicelabelptr=slicelabel;
      }
      if(Match(buffer,"SLCT") == 1){
        terrain=1;
      }
      if(Match(buffer,"SLFL") == 1){
        terrain=1;
        fire_line=1;
      }
      if(Match(buffer,"SLCC") == 1){
        cellcenter_slice_active = 1;
        cellcenter=1;
      }
      if(Match(buffer, "SLCD") == 1){
        facecenter_slice_active = 1;
        facecenter = 1;
      }
      TrimBack(buffer);
      len=strlen(buffer);
      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(len>5){
        char *buffer3;

        buffer3=buffer+4;
        sscanf(buffer3,"%i %f",&blocknumber,&above_ground_level);
        blocknumber--;
      }
      if(FGETS(buffer,255,stream)==NULL){
        nsliceinfo--;
        BREAK;
      }

      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);

      sd = sliceinfo + nn_slice - 1;
      sd->ntimes = 0;
      sd->ntimes_old = 0;
      sd->globalmax = -1.0e30;
      sd->globalmin = -sd->globalmax;
      sd->sliceoffset_fds = sliceoffset_fds;
      sd->reg_file=NULL;
      sd->comp_file=NULL;
      sd->vol_file=NULL;
      sd->slicelabel=NULL;
      sd->slicetype=SLICE_NODE_CENTER;
      if(terrain==1){
        sd->slicetype=SLICE_TERRAIN;
      }
      if(fire_line==1)sd->slicetype=SLICE_FIRELINE;
      if(cellcenter==1){
        sd->slicetype=SLICE_CELL_CENTER;
      }
      if(facecenter == 1){
        sd->slicetype = SLICE_FACE_CENTER;
      }

      islicecount++;
      strcpy(buffer2,bufferptr);
      strcat(buffer2,".svz");
      has_reg=0;
      has_comp=0;
      if(lookfor_zip==1&&FILE_EXISTS_CASEDIR(buffer2)==YES)has_comp=1;
      if(has_comp==0&&(fast_startup==1||FILE_EXISTS_CASEDIR(bufferptr)==YES))has_reg=1;
      if(has_reg==0&&has_comp==0){
        nsliceinfo--;
        nslicefiles--;
        nn_slice--;
        if(FGETS(buffer,255,stream)==NULL){
          BREAK;
        }
        if(FGETS(buffer,255,stream)==NULL){
          BREAK;
        }
        if(FGETS(buffer,255,stream)==NULL){
          BREAK;
        }
        continue;
      }

      NewMemory((void **)&sd->reg_file,(unsigned int)(len+1));
      STRCPY(sd->reg_file,bufferptr);

      NewMemory((void **)&sd->comp_file,(unsigned int)(len+4+1));
      STRCPY(sd->comp_file,buffer2);

      sd->compression_type=UNCOMPRESSED;
      if(has_comp==1){
        sd->compression_type=COMPRESSED_ZLIB;
        sd->file=sd->comp_file;
      }
      if(sd->compression_type==UNCOMPRESSED){
        sd->file=sd->reg_file;
      }

      if(sd->slicetype==SLICE_TERRAIN){
        if(ReadLabels(&sd->label,stream,"(terrain)")==2)return 2;
      }
      else if(sd->slicetype==SLICE_CELL_CENTER){
        if(ReadLabels(&sd->label,stream,"(cell centered)")==2)return 2;
      }
      else if(sd->slicetype == SLICE_FACE_CENTER){
        if(ReadLabels(&sd->label, stream,"(face centered)") == 2)return 2;
      }
      else{
        if(ReadLabels(&sd->label,stream,NULL)==2)return 2;
      }
#ifdef pp_COLORBARFLIP
      if(strlen(sd->label.longlabel)>14&&
         strncmp(sd->label.longlabel,"SOOT VISIBILITY",15)==0){
         sd->colorbar_autoflip=1;
      }
      else{
         sd->colorbar_autoflip=0;
      }
#endif


      {
        char volfile[1024];

        strcpy(volfile,bufferptr);
        strcat(volfile,".svv");
        sd->vol_file=NULL;
        if(FILE_EXISTS_CASEDIR(volfile)==YES){
          NewMemory((void **)&sd->vol_file,(unsigned int)(len+4+1));
          STRCPY(sd->vol_file,volfile);
          have_volcompressed=1;
        }
      }

      NewMemory((void **)&sd->size_file,(unsigned int)(len+3+1));
      STRCPY(sd->size_file,bufferptr);
      STRCAT(sd->size_file,".sz");

      sd->slicelabel=NULL;
      if(slicelabelptr!=NULL){
        int lenslicelabel;

        lenslicelabel=strlen(slicelabel)+1;
        NewMemory((void **)&sd->slicelabel,lenslicelabel);
        strcpy(sd->slicelabel,slicelabel);
      }
      if(read_slice_header==1){
        int error;

        FORTgetsliceheader(sd->file,&ii1,&ii2,&jj1,&jj2,&kk1,&kk2,&error,strlen(sd->file));
      }
      sd->is1=ii1;
      sd->is2=ii2;
      sd->js1=jj1;
      sd->js2=jj2;
      sd->ks1=kk1;
      sd->ks2=kk2;
      sd->ijk_min[0] = ii1;
      sd->ijk_max[0] = ii2;
      sd->ijk_min[1] = jj1;
      sd->ijk_max[1] = jj2;
      sd->ijk_min[2] = kk1;
      sd->ijk_max[2] = kk2;
      sd->is_fed=0;
      sd->above_ground_level=above_ground_level;
      sd->seq_id=nn_slice;
      sd->autoload=0;
      sd->display=0;
      sd->loaded=0;
      sd->loading = 0;
      sd->qslicedata=NULL;
      sd->compindex=NULL;
      sd->slicecomplevel=NULL;
      sd->qslicedata_compressed=NULL;
      if(sd->is1!=sd->is2&&sd->js1!=sd->js2&&sd->ks1!=sd->ks2){
        sd->volslice=1;
      }
      else{
        sd->volslice = 0;
      }
      sd->times=NULL;
      sd->slicelevel=NULL;
      sd->iqsliceframe=NULL;
      sd->qsliceframe=NULL;
      sd->timeslist=NULL;
      sd->blocknumber=blocknumber;
      sd->vloaded=0;
      sd->reload=0;
      sd->nline_contours=0;
      sd->line_contours=NULL;
      sd->menu_show=1;
      sd->constant_color=NULL;
      sd->histograms = NULL;
      sd->nhistograms=0;
      {
        meshdata *meshi;

        meshi = meshinfo + blocknumber;
        sd->mesh_type=meshi->mesh_type;
        sd->full_mesh = NO;
        if(sd->is2 - sd->is1 == meshi->ibar &&
           sd->js2 - sd->js1 == meshi->jbar &&
           sd->ks2 - sd->ks1 == meshi->kbar)sd->full_mesh = YES;
      }

      if(IsSliceDup(sd,nn_slice)==1){
        FREEMEMORY(sd->reg_file);
        FREEMEMORY(sd->comp_file);
        FREEMEMORY(sd->vol_file);
        FREEMEMORY(sd->slicelabel);

        nsliceinfo--;
        nslicefiles--;
        nn_slice--;
        continue;
      }
      sliceinfo_copy++;
      continue;
    }
  /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ BNDF ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */
    if(Match(buffer, "BNDF") == 1 || Match(buffer, "BNDC") == 1 || Match(buffer, "BNDE") == 1 || Match(buffer, "BNDS")==1){
      patchdata *patchi;
      int version;
      int blocknumber;
      size_t len;
      char *geom_fdsfiletype;

      if(setup_only == 1)continue;
      nn_patch++;

      TrimBack(buffer);
      len=strlen(buffer);

      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      version=0;
      if(len>5){
        char *buffer3;

        buffer3=buffer+4;
        sscanf(buffer3,"%i %i",&blocknumber,&version);
        blocknumber--;
      }
      patchi = patchinfo + ipatch;

      patchi->version=version;
      patchi->ntimes = 0;
      patchi->ntimes_old = 0;
      strcpy(patchi->scale, "");
      patchi->geom_fdsfiletype=NULL;
      patchi->filetype = PATCH_NODE_CENTER;
      patchi->geom_smvfiletype = PATCH_STRUCTURED;
      patchi->slice = 0;
      if(Match(buffer,"BNDC") == 1){
        patchi->filetype = PATCH_CELL_CENTER;
      }
      if(Match(buffer,"BNDE") == 1){
        ngeom_data++;
        patchi->filetype=PATCH_GEOMETRY;
        patchi->geom_smvfiletype=PATCH_GEOMETRY_BOUNDARY;
      }
      if(Match(buffer, "BNDS") == 1){
        char *sliceparms;

        ngeom_data++;
        patchi->filetype = PATCH_GEOMETRY;
        patchi->geom_smvfiletype = PATCH_GEOMETRY_SLICE;
        patchi->slice = 1;

        sliceparms = strchr(buffer, '&');
        if(sliceparms != NULL){
          int ijk[6],j;

          sliceparms++;
          sliceparms[-1] = 0;
          sscanf(sliceparms, "%i %i %i %i %i %i", ijk,ijk+1,ijk+2,ijk+3,ijk+4,ijk+5);
          for(j=0;j<6;j++){
            patchi->ijk[j]=ijk[j];
          }
        }

        geom_fdsfiletype = strchr(buffer, '#');
        if(geom_fdsfiletype != NULL){
          int len_geom_fdsfiletype;

          geom_fdsfiletype++;
          geom_fdsfiletype[-1] = 0;
          geom_fdsfiletype = TrimFrontBack(geom_fdsfiletype);
          len_geom_fdsfiletype = strlen(geom_fdsfiletype);
          if(len_geom_fdsfiletype>0){
            NewMemory((void **)&patchi->geom_fdsfiletype,(unsigned int)(len_geom_fdsfiletype+1));
            strcpy(patchi->geom_fdsfiletype,geom_fdsfiletype);
          }
        }
      }

      if(FGETS(buffer,255,stream)==NULL){
        npatchinfo--;
        BREAK;
      }

      bufferptr=TrimFrontBack(buffer);
      len=strlen(bufferptr);
      NewMemory((void **)&patchi->reg_file,(unsigned int)(len+1));
      STRCPY(patchi->reg_file,bufferptr);

      NewMemory((void **)&patchi->comp_file,(unsigned int)(len+4+1));
      STRCPY(patchi->comp_file,bufferptr);
      STRCAT(patchi->comp_file,".svz");

      NewMemory((void **)&patchi->size_file,(unsigned int)(len+4+1));
      STRCPY(patchi->size_file,bufferptr);
//      STRCAT(patchi->size_file,".szz"); when we actully use file check both .sz and .szz extensions

      if(FILE_EXISTS_CASEDIR(patchi->comp_file)==YES){
        patchi->compression_type=COMPRESSED_ZLIB;
        patchi->file=patchi->comp_file;
      }
      else{
        patchi->compression_type=UNCOMPRESSED;
        patchi->file=patchi->reg_file;
      }

      patchi->geomfile=NULL;
      patchi->geominfo=NULL;
      if(patchi->filetype==PATCH_GEOMETRY){
        int igeom;

        if(FGETS(buffer,255,stream)==NULL){
          npatchinfo--;
          BREAK;
        }
        bufferptr=TrimFrontBack(buffer);
        NewMemory((void **)&patchi->geomfile,strlen(bufferptr)+1);
        strcpy(patchi->geomfile,bufferptr);
        for(igeom=0;igeom<ngeominfo;igeom++){
          geomdata *geomi;

          geomi = geominfo + igeom;
          if(strcmp(geomi->file,patchi->geomfile)==0){
            patchi->geominfo=geomi;
            if(patchi->slice == 0){
              geomi->geomtype = GEOM_BOUNDARY;
              geomi->fdsblock = FDSBLOCK;
            }
            else{
              geomi->geomtype = GEOM_SLICE;
              geomi->fdsblock = NOT_FDSBLOCK;
            }
            break;
          }
        }
      }
      patchi->modtime=0;
      patchi->geom_timeslist=NULL;
      patchi->geom_ivals_dynamic=NULL;
      patchi->geom_ivals_static=NULL;
      patchi->geom_ndynamics=NULL;
      patchi->geom_nstatics=NULL;
      patchi->geom_times=NULL;
      patchi->geom_vals=NULL;
      patchi->geom_ivals=NULL;
      patchi->geom_nvals=0;
      patchi->histogram = NULL;
      patchi->blocknumber=blocknumber;
      patchi->seq_id=nn_patch;
      patchi->autoload=0;
      patchi->loaded=0;
      patchi->display=0;
      patchi->inuse=0;
      patchi->inuse_getbounds=0;
      patchi->bounds.defined=0;
      patchi->unit_start=unit_start++;
      patchi->setchopmin=0;
      patchi->chopmin=1.0;
      patchi->setchopmax=0;
      patchi->chopmax=0.0;
      meshinfo[blocknumber].patchfilenum=-1;
      if(fast_startup==1||FILE_EXISTS_CASEDIR(patchi->file)==YES){
        char geomlabel2[256], *geomptr=NULL;

        strcpy(geomlabel2, "");
        if(patchi->filetype==PATCH_CELL_CENTER){
          if(ReadLabels(&patchi->label,stream,"(cell centered)")==2)return 2;
        }
        else if(patchi->filetype==PATCH_NODE_CENTER){
          if(ReadLabels(&patchi->label,stream,NULL)==2)return 2;
        }
        else if(patchi->filetype==PATCH_GEOMETRY){
          char geomlabel[256];

          strcpy(geomlabel, "(geometry)");
          if(patchi->geom_fdsfiletype != NULL){
            if(strcmp(patchi->geom_fdsfiletype, "EXIMBND_FACES") == 0){
              strcat(geomlabel, " - EXIM faces");
              strcpy(geomlabel2, " - EXIM faces");
            }
            if(strcmp(patchi->geom_fdsfiletype, "CUT_CELLS") == 0){
              strcat(geomlabel, " - Cut cell faces");
              strcpy(geomlabel2, " - Cut cell faces");
            }
          }
          if(ReadLabels(&patchi->label,stream,geomlabel)==2)return 2;
        }
        strcpy(patchi->menulabel_base, patchi->label.longlabel);
        if(strlen(geomlabel2) > 0){
          geomptr = strstr(patchi->menulabel_base, geomlabel2);
          if(geomptr != NULL)geomptr[0] = 0;
        }
        NewMemory((void **)&patchi->histogram,sizeof(histogramdata));
        InitHistogram(patchi->histogram,NHIST_BUCKETS, NULL, NULL);
        ipatch++;
      }
      else{
        if(trainer_mode==0){
          fprintf(stderr,"*** Error: The file %s does not exist\n",buffer);
        }
        if(ReadLabels(&patchi->label,stream,NULL)==2)return 2;
        npatchinfo--;
      }
      continue;
    }

    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ ISOF ++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  */

    if(Match(buffer,"ISOF") == 1||Match(buffer,"TISOF")==1||Match(buffer,"ISOG") == 1){
      isodata *isoi;
      int get_isolevels;
      int dataflag=0,geomflag=0;
      char tbuffer[255], *tbufferptr;
      int blocknumber;
      size_t len;
      char *buffer3;

      if(setup_only == 1)continue;
      isoi = isoinfo + iiso;
      isoi->isof_index = nn_iso%nisos_per_mesh;
      nn_iso++;

      if(Match(buffer, "TISOF") == 1)dataflag = 1;
      if(Match(buffer,"ISOG")==1)geomflag=1;
      TrimBack(buffer);
      len=strlen(buffer);

      if(nmeshes>1){
        blocknumber=ioffset-1;
      }
      else{
        blocknumber=0;
      }
      if(len>5&&dataflag==0){
        buffer3=buffer+4;
        sscanf(buffer3,"%i",&blocknumber);
        blocknumber--;
      }
      if(len>6&&dataflag==1){
        buffer3=buffer+5;
        sscanf(buffer3,"%i",&blocknumber);
        blocknumber--;
      }
      if(FGETS(buffer,255,stream)==NULL){
        nisoinfo--;
        BREAK;
      }

      isoi->tfile=NULL;
      isoi->seq_id=nn_iso;
      isoi->autoload=0;
      isoi->blocknumber=blocknumber;
      isoi->loaded=0;
      isoi->loading = 0;
      isoi->display=0;
      isoi->dataflag=dataflag;
      isoi->geomflag=geomflag;
      isoi->nlevels=0;
      isoi->levels=NULL;
      isoi->is_fed=0;
      isoi->memory_id = ++nmemory_ids;

      isoi->normaltable=NULL;
      isoi->color_label.longlabel=NULL;
      isoi->color_label.shortlabel=NULL;
      isoi->color_label.unit=NULL;
      isoi->geominfo=NULL;
      NewMemory((void **)&isoi->geominfo,sizeof(geomdata));
      nmemory_ids++;
      isoi->geominfo->memory_id=nmemory_ids;
      InitGeom(isoi->geominfo,GEOM_ISO,NOT_FDSBLOCK);

      bufferptr=TrimFrontBack(buffer);

      len=strlen(bufferptr);

      NewMemory((void **)&isoi->reg_file,(unsigned int)(len+1));
      STRCPY(isoi->reg_file,bufferptr);

      NewMemory((void **)&isoi->size_file,(unsigned int)(len+3+1));
      STRCPY(isoi->size_file,bufferptr);
      STRCAT(isoi->size_file,".sz");

      if(dataflag==1&&geomflag==1){
        if(FGETS(tbuffer,255,stream)==NULL){
          nisoinfo--;
          BREAK;
        }
        TrimBack(tbuffer);
        tbufferptr=TrimFront(tbuffer);
        NewMemory((void **)&isoi->tfile,strlen(tbufferptr)+1);
        strcpy(isoi->tfile,tbufferptr);
      }

      if(fast_startup==1||FILE_EXISTS_CASEDIR(isoi->reg_file)==YES){
        get_isolevels=1;
        isoi->file=isoi->reg_file;
        if(ReadLabels(&isoi->surface_label,stream,NULL)==2)return 2;
        if(geomflag==1){
          int ntimes_local;
          geomdata *geomi;
          float **colorlevels,*levels;

          isoi->geominfo->file=isoi->file;
          geomi = isoi->geominfo;
          geomi->file=isoi->file;
          ReadGeomHeader(geomi,NULL,&ntimes_local);
          isoi->nlevels=geomi->nfloat_vals;
          if(isoi->nlevels>0){
            NewMemory((void **)&levels,isoi->nlevels*sizeof(float));
            NewMemory((void **)&colorlevels,isoi->nlevels*sizeof(float *));
            for(i=0;i<isoi->nlevels;i++){
              colorlevels[i]=NULL;
              levels[i]=geomi->float_vals[i];
            }
            isoi->levels=levels;
            isoi->colorlevels=colorlevels;
          }
        }
        else{
          GetIsoLevels(isoi->file,dataflag,&isoi->levels,&isoi->colorlevels,&isoi->nlevels);
        }
        if(dataflag==1){
          if(ReadLabels(&isoi->color_label,stream,NULL)==2)return 2;
        }
        iiso++;
      }
      else{
        get_isolevels=0;
        if(trainer_mode==0){
          fprintf(stderr,"*** Error: the file %s does not exist\n",buffer);
        }

        if(ReadLabels(&isoi->surface_label,stream,NULL)==2)return 2;
        if(dataflag==1){
          if(ReadLabels(&isoi->color_label,stream,NULL)==2)return 2;
        }
        nisoinfo--;
      }
      if(get_isolevels==1){
        int len_clevels;
        char clevels[1024];

        Array2String(isoi->levels,isoi->nlevels,clevels);
        len_clevels = strlen(clevels);
        if(len_clevels>0){
          int len_long;
          char *long_label, *unit_label;

          long_label = isoi->surface_label.longlabel;
          unit_label = isoi->surface_label.unit;
          len_long = strlen(long_label)+strlen(unit_label)+len_clevels+3+1;
          if(dataflag==1)len_long+=(strlen(isoi->color_label.longlabel)+15+1);
          ResizeMemory((void **)&long_label,(unsigned int)len_long);
          isoi->surface_label.longlabel=long_label;
          strcat(long_label,": ");
          strcat(long_label,clevels);
          strcat(long_label," ");
          strcat(long_label,unit_label);
          if(dataflag==1){
            strcat(long_label," (Colored by: ");
            strcat(long_label,isoi->color_label.longlabel);
            strcat(long_label,")");
          }
          TrimBack(long_label);
        }
      }
      continue;
    }

  }

  STOP_TIMER(pass4_time);

/*
   ************************************************************************
   ************************ end of pass 4 *********************************
   ************************************************************************
 */

  START_TIMER(pass5_time);

  if(autoterrain==1){
    float zbarmin;

    zbarmin=meshinfo->xyz_bar0[ZZZ];
    for(i=1;i<nmeshes;i++){
      meshdata *meshi;

      meshi = meshinfo + i;
      if(meshi->xyz_bar0[ZZZ]<zbarmin)zbarmin=meshi->xyz_bar0[ZZZ];
    }
    nOBST=0;
    iobst=0;
    for(i=0;i<nmeshes;i++){
      meshdata *meshi;
      int ibar, jbar;

      meshi = meshinfo + i;
      ibar = meshi->ibar;
      jbar = meshi->jbar;
      if(ibar>0&&jbar>0){
        float *zcell;
        int j;

        NewMemory((void **)&meshi->zcell,ibar*jbar*sizeof(float));
        zcell = meshi->zcell;
        for(j=0;j<ibar*jbar;j++){
          zcell[j]=zbarmin; // assume initially that zcell (terrain height) is at base of the domain
        }
      }
    }
  }

  /*
   ************************************************************************
   ************************ start of pass 5 *******************************
   ************************************************************************
 */

#ifdef pp_READBUFFER
  REWIND(stream);
#else
  REWIND(stream1);
  if(stream2!=NULL)rewind(stream2);
  stream=stream1;
#endif
  if(do_pass4==1||autoterrain==1){
    do_pass5 = 1;
    PRINTF("%s","  pass 5\n");
  }

  while((autoterrain==1||do_pass4==1)){
    if(FEOF(stream)!=0){
      BREAK;
    }

    if(FGETS(buffer,255,stream)==NULL){
      BREAK;
    }
    if(strncmp(buffer," ",1)==0||buffer[0]==0)continue;

    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ MINMAXBNDF +++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    */

    if(Match(buffer, "MINMAXBNDF")==1){
      char *file_ptr, file2_local[1024];
      float valmin, valmax;
      float percentile_min, percentile_max;

      FGETS(buffer,255,stream);
      strcpy(file2_local,buffer);
      file_ptr = file2_local;
      TrimBack(file2_local);
      file_ptr = TrimFront(file2_local);

      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f %f",&valmin,&valmax,&percentile_min,&percentile_max);

      for(i=0;i<npatchinfo;i++){
        patchdata *patchi;

        patchi = patchinfo + i;
        if(strcmp(file_ptr,patchi->file)==0){
          patchi->diff_valmin=percentile_min;
          patchi->diff_valmax=percentile_max;
          break;
        }
      }
      continue;
    }
    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ MINMAXSLCF +++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    */
    if(Match(buffer, "MINMAXSLCF")==1){
      char *file_ptr, file2_local[1024];
      float valmin, valmax;
      float percentile_min, percentile_max;

      FGETS(buffer,255,stream);
      strcpy(file2_local,buffer);
      file_ptr = file2_local;
      TrimBack(file2_local);
      file_ptr = TrimFront(file2_local);

      FGETS(buffer,255,stream);
      sscanf(buffer,"%f %f %f %f",&valmin,&valmax,&percentile_min,&percentile_max);

      for(i=0;i<nsliceinfo;i++){
        slicedata *slicei;

        slicei = sliceinfo + i;
        if(strcmp(file_ptr,slicei->file)==0){
          slicei->diff_valmin=percentile_min;
          slicei->diff_valmax=percentile_max;
          break;
        }
      }
      continue;
    }
    /*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ++++++++++++++++++++++ OBST +++++++++++++++++++++++++++++++++
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    */
    if(Match(buffer, "OBST")==1&&autoterrain==1){
      meshdata *meshi;
      int nxcell;
      int n_blocks;
      int iblock;
      int nn;

      nOBST++;
      iobst++;
      FGETS(buffer,255,stream);
      sscanf(buffer,"%i",&n_blocks);


      if(n_blocks<=0)n_blocks=0;
      if(n_blocks==0)continue;

      meshi=meshinfo+iobst-1;

      for(nn=0;nn<n_blocks;nn++){
        FGETS(buffer,255,stream);
      }
      nxcell = meshi->ibar;
      nn=-1;
      for(iblock=0;iblock<n_blocks;iblock++){
        int ijk2[5],kmax;
        int ii, jj;
        float block_zmax;

        if(meshi->is_block_terrain!=NULL&&meshi->is_block_terrain[iblock]==0){
          FGETS(buffer,255,stream);
          continue;
        }
        nn++;

        FGETS(buffer,255,stream);
        sscanf(buffer,"%i %i %i %i %i %i",ijk2,ijk2+1,ijk2+2,ijk2+3,ijk2+4,&kmax);
        block_zmax = meshi->zplt[kmax];
        for(ii=ijk2[0];ii<ijk2[1];ii++){
          for(jj=ijk2[2];jj<ijk2[3];jj++){
            int ij;

            ij = IJCELL2(ii,jj);
            meshi->zcell[ij]=MAX(meshi->zcell[ij],block_zmax);
          }
        }
      }
      FREEMEMORY(meshi->is_block_terrain);
      continue;
    }
  }
  PrintMemoryInfo;
  if(do_pass5==1){
    STOP_TIMER(pass5_time);
  }
  else{
    pass5_time = 0.0;
  }

/*
   ************************************************************************
   ************************ wrap up ***************************************
   ************************************************************************
 */

  STOP_TIMER(processing_time);
  START_TIMER(wrapup_time);

  PRINTF("  wrapping up\n");
  CheckMemory;
  UpdateIsoColors();
  CheckMemory;

  UpdateSmoke3dFileParms();

  //RemoveDupBlockages();
  InitCullGeom(cullgeom);
  InitEvacProp();

  UpdateINIList();

  if(meshinfo!=NULL&&meshinfo->jbar==1)force_isometric=1;

// update csv data

  if(hrr_csv_filename!=NULL)ReadHRR(LOAD, &errorcode);
  ReadDeviceData(NULL,CSV_FDS,UNLOAD);
  ReadDeviceData(NULL,CSV_EXP,UNLOAD);
  for(i=0;i<ncsvinfo;i++){
    csvdata *csvi;

    csvi = csvinfo + i;
    if(csvi->type==CSVTYPE_DEVC)ReadDeviceData(csvi->file,CSV_FDS,LOAD);
    if(csvi->type==CSVTYPE_EXT)ReadDeviceData(csvi->file,CSV_EXP,LOAD);
  }
  SetupDeviceData();
  if(nzoneinfo>0)SetupZoneDevs();

  InitMultiThreading();

  InitPartProp();

  InitClip();

  if(noutlineinfo>0){
    highlight_flag=2;
  }
  else{
    highlight_flag=1;
  }
  InitCadColors();

  // update loaded lists

  FREEMEMORY(slice_loaded_list);
  if(nsliceinfo>0){
    NewMemory((void **)&slice_loaded_list,nsliceinfo*sizeof(int));
  }

  FREEMEMORY(slice_sorted_loaded_list);
  if(nsliceinfo>0){
    NewMemory((void **)&slice_sorted_loaded_list, nsliceinfo*sizeof(int));
  }

  FREEMEMORY(patch_loaded_list);
  if(npatchinfo>0){
    NewMemory((void **)&patch_loaded_list,npatchinfo*sizeof(int));
  }
  UpdateLoadedLists();
  CheckMemory;

  UpdateMeshCoords();
  CheckMemory;

  /*
    Associate a surface with each block.
  */
  UpdateUseTextures();
  CheckMemory;

  /* compute global bar's and box's */


  for(i=0;i<npartclassinfo;i++){
    partclassdata *partclassi;

    partclassi = partclassinfo + i;

    if(partclassi->device_name!=NULL){
        float length, azimuth, elevation;

        partclassi->diameter=SCALE2SMV(partclassi->diameter);
        partclassi->length=SCALE2SMV(partclassi->length);
        length=partclassi->length;
        azimuth = partclassi->azimuth*DEG2RAD;
        elevation = partclassi->elevation*DEG2RAD;
        partclassi->dx = cos(azimuth)*cos(elevation)*length/2.0;
        partclassi->dy = sin(azimuth)*cos(elevation)*length/2.0;
        partclassi->dz =              sin(elevation)*length/2.0;
    }
  }

  shooter_xyz[0]=xbar/2.0;
  shooter_xyz[1] = 0.0;
  shooter_xyz[2] = zbar/2.0;
  shooter_dxyz[0]=xbar/4.0;
  shooter_dxyz[1]=0.0;
  shooter_dxyz[2]=0.0;
  shooter_nparts=100;
  shooter_velmag=1.0;
  shooter_veldir=0.0;
  shooter_fps=10;
  shooter_vel_type=1;

  UpdatePlotxyzAll();
  CheckMemory;

  UpdateVSlices();
  if(update_slice==1)return 3;

  GetBoundaryParams();

  GetGSliceParams();

  active_smokesensors=0;
  for(i=0;i<ndeviceinfo;i++){
    devicedata *devicei;
    char *label;

    devicei = deviceinfo + i;
    devicei->device_mesh= GetMeshNoFail(devicei->xyz);
    label = devicei->object->label;
    if(strcmp(label,"smokesensor")==0){
      active_smokesensors=1;
    }
    if(devicei->plane_surface!=NULL){
      InitDevicePlane(devicei);
    }
  }

  MakeIBlankCarve();
  MakeIBlankSmoke3D();
  MakeIBlankAll();
#ifdef pp_THREADIBLANK
  if(runscript == 1){
    JOIN_IBLANK
  }
#endif
  LOCK_IBLANK
  SetVentDirs();
  UNLOCK_IBLANK
  UpdateFaces();

  xcenGLOBAL=xbar/2.0;  ycenGLOBAL=ybar/2.0; zcenGLOBAL=zbar/2.0;
  xcenCUSTOM=xbar/2.0;  ycenCUSTOM=ybar/2.0; zcenCUSTOM=zbar/2.0;
  glui_rotation_index = nmeshes;

  UpdateEndianInfo();

  UpdateBoundInfo();

  UpdateObjectUsed();

  // close .smv file

#ifdef pp_READBUFFER
  FCLOSE(stream);
#else
  FCLOSE(stream1);
  if(stream2!=NULL)fclose(stream2);
  stream = NULL;
#endif

  UpdateSelectFaces();
  UpdateSliceTypes();
  UpdateSliceBoundLabels();
  UpdateIsoTypes();
  UpdateBoundaryTypes();
  if(autoterrain==1){
    for(i=0;i<nmeshes;i++){
      meshdata *meshi;
      float *zcell;
      int j;

      meshi = meshinfo + i;
      zcell = meshi->zcell;

      zterrain_min = zcell[0];
      zterrain_max = zterrain_min;
      for(j=1;j<meshi->ibar*meshi->jbar;j++){
        float zval;

        zval=*zcell++;
        if(zval<zterrain_min)zterrain_min=zval;
        if(zval>zterrain_max)zterrain_max=zval;
      }
    }
  }
  UpdateTerrain(1,vertical_factor);
  UpdateTerrainColors();
  UpdateSmoke3dMenuLabels();
  UpdateVSliceTypes();
  UpdateBoundaryMenuLabels();
  UpdateIsoMenuLabels();
  UpdatePartMenuLabels();
  UpdateTourMenuLabels();
  InitUserTicks();
  clip_I=ibartemp; clip_J=jbartemp; clip_K=kbartemp;

  // define changed_idlist used for blockage editing

  {
    int ntotal=0;

    for(i=0;i<nmeshes;i++){
      meshdata *meshi;

      meshi = meshinfo + i;
      ntotal += meshi->nbptrs;
    }
    FREEMEMORY(changed_idlist);

    NewMemory((void **)&changed_idlist,sizeof(int)*(ntotal+1));

    for(i=0;i<ntotal;i++){
      changed_idlist[i]=0;
    }
    nchanged_idlist=ntotal;
  }

  InitVolRender();
  InitVolRenderSurface(FIRSTCALL);
  radius_windrose = 0.2*xyzmaxdiff;

#ifdef pp_CULL

  // define data structures used to speed up 3d smoke drawing (by culling non-visible smoke planes)

  if(cullactive==1)InitCull(cullsmoke);
#endif
  UpdateMeshTerrain();

  ReadAllGeom();
  ngeominfoptrs=0;
  GetGeomInfoPtrs(&geominfoptrs,&ngeominfoptrs);
  UpdateTriangles(GEOM_STATIC,GEOM_UPDATE_ALL);
  GetFaceInfo();

  SetupMeshWalls();

  PRINTF("%s",_("complete"));
  PRINTF("\n\n");
  PrintMemoryInfo;

  STOP_TIMER(wrapup_time);
  PRINTF("\n");
  PRINTF(".smv Processing Times\n");
  PRINTF("---------------------\n");

#ifdef pp_TIMINGS
#define ALWAYS (1==1)||
#else
#define ALWAYS
#endif

  if(ALWAYS read_time>1.0)         PRINTF(".smv file(net): %.1f s\n", read_time);
  if(ALWAYS getfilelist_time>1.0)  PRINTF("      filelist: %.1f s\n", getfilelist_time);
  if(ALWAYS read_time_elapsed>1.0) PRINTF(".smv file(cum): %.1f s\n", read_time_elapsed);
  if(ALWAYS pass0_time>1.0)        PRINTF("         setup: %.1f s\n", pass0_time);
  if(ALWAYS pass1_time>1.0)        PRINTF("        pass 1: %.1f s\n", pass1_time);
  if(ALWAYS pass2_time>1.0)        PRINTF("        pass 2: %.1f s\n", pass2_time);
  if(ALWAYS pass3_time>1.0)        PRINTF("        pass 3: %.1f s\n", pass3_time);
  if(ALWAYS pass4_time>1.0)        PRINTF("        pass 4: %.1f s\n", pass4_time);
  if(ALWAYS pass5_time>1.0)        PRINTF("        pass 5: %.1f s\n", pass5_time);
                         PRINTF("all passes: %.1f s\n", processing_time);
  if(ALWAYS wrapup_time>1.0)    PRINTF("   wrap up: %.1f s\n", wrapup_time);
  PRINTF("\n");
  return 0;
}

/* ------------------ UpdateUseTextures ------------------------ */

void UpdateUseTextures(void){
  int i;

  for(i=0;i<ntextures;i++){
    texturedata *texti;

    texti=textureinfo + i;
    texti->used=0;
  }
  for(i=0;i<nmeshes;i++){
    meshdata *meshi;
    int j;

    meshi=meshinfo + i;
    if(textureinfo!=NULL){
      for(j=0;j<meshi->nbptrs;j++){
        int k;
        blockagedata *bc;

        bc=meshi->blockageinfoptrs[j];
        for(k=0;k<6;k++){
          texturedata *texti;

          texti = bc->surf[k]->textureinfo;
          if(texti!=NULL&&texti->loaded==1){
            if(usetextures==1)texti->display=1;
            texti->used=1;
          }
        }
      }
    }
    for(j=0;j<meshi->nvents+12;j++){
      ventdata *vi;

      vi = meshi->ventinfo + j;
      if(vi->surf[0]==NULL){
        if(vent_surfacedefault!=NULL){
          if(j>=meshi->nvents+6){
            vi->surf[0]=exterior_surfacedefault;
          }
          else{
            vi->surf[0]=vent_surfacedefault;
          }
        }
      }
      if(textureinfo!=NULL){
        if(vi->surf[0]!=NULL){
          texturedata *texti;

          texti = vi->surf[0]->textureinfo;
          if(texti!=NULL&&texti->loaded==1){
            if(usetextures==1)texti->display=1;
            texti->used=1;
          }
        }
      }
    }
  }
  for(i=0;i<ndevice_texture_list;i++){
    int texture_index;
    texturedata *texti;

    texture_index  = device_texture_list_index[i];
    texti=textureinfo + texture_index;
    if(texti!=NULL&&texti->loaded==1){
      if(usetextures==1)texti->display=1;
      texti->used=1;
    }
  }
  for(i=0;i<ngeominfo;i++){
    geomdata *geomi;

    geomi = geominfo + i;
    if(textureinfo!=NULL&&geomi->surf!=NULL){
        texturedata *texti;

      texti = geomi->surf->textureinfo;
      if(texti!=NULL&&texti->loaded==1){
        if(usetextures==1)texti->display=1;
        texti->used=1;
      }
    }
  }
  ntextures_loaded_used=0;
  for(i=0;i<ntextures;i++){
    texturedata *texti;

    texti = textureinfo + i;
    if(texti->loaded==0)continue;
    if(texti->used==0)continue;
    ntextures_loaded_used++;
  }
}

/* ------------------ ReadIni2 ------------------------ */

int ReadIni2(char *inifile, int localfile){
  int i;
  FILE *stream;

  updatemenu = 1;
  updatefacelists = 1;

  if((stream = fopen(inifile, "r")) == NULL)return 1;
  PRINTF("%s", _("processing config file: "));
  PRINTF("%s\n", inifile);

  for(i = 0; i<nunitclasses_ini; i++){
    f_units *uc;

    uc = unitclasses_ini + i;
    FREEMEMORY(uc->units);
  }
  FREEMEMORY(unitclasses_ini);
  nunitclasses_ini = 0;

  if(localfile == 1){
    UpdateINIList();
  }

  if(localfile == 1){
    update_selectedtour_index = 0;
  }

  /* find number of each kind of file */

  while(!feof(stream)){
    char buffer[255], buffer2[255];

    CheckMemory;
    if(fgets(buffer, 255, stream) == NULL)break;

   if(Match(buffer, "GEOMDOMAIN") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i ", &showgeom_inside_domain, &showgeom_outside_domain);
      showgeom_inside_domain = CLAMP(showgeom_inside_domain, 0, 1);
      showgeom_outside_domain = CLAMP(showgeom_outside_domain, 0, 1);
      continue;
   }
   if(Match(buffer, "CO2COLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i", global_co2color,global_co2color+1,global_co2color+2);
      global_co2color[0] = CLAMP(global_co2color[0], 0, 2550);
      global_co2color[1] = CLAMP(global_co2color[1], 0, 2550);
      global_co2color[2] = CLAMP(global_co2color[2], 0, 2550);
        continue;
    }
    if(Match(buffer, "SLICEDUP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i", &slicedup_option, &vectorslicedup_option,&boundaryslicedup_option);
      continue;
    }
    if(Match(buffer, "BLENDMODE")==1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i,%i", &slices3d_max_blending, &hrrpuv_max_blending,&showall_3dslices);
    }
    if(Match(buffer, "FIREPARAMS")==1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %f %f %f %f %f %f %f %f",
      &slicehrrpuv_lower, &slicehrrpuv_middle, &slicehrrpuv_upper,
      &slicehrrpuv_cut1, &slicehrrpuv_cut2, &slicehrrpuv_offset,
      &voltemp_factor,&voltemp_offset);
      continue;
    }
    if(Match(buffer, "FREEZEVOLSMOKE")==1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i", &freeze_volsmoke,&autofreeze_volsmoke);
      continue;
    }
    if(Match(buffer, "SHOWMESHMENUS")==1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i", &show_meshmenus);
      continue;
    }
    if(Match(buffer, "GEOMCELLPROPS")==1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i",
        &immersed_celltype);
      immersed_celltype = CLAMP(immersed_celltype,0,MAX_CELL_TYPES-1);

      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i",
        immersed_edgetypes,immersed_edgetypes+1,immersed_edgetypes+2);

      for(i=0;i<3;i++){
        immersed_edgetypes[i] = CLAMP(immersed_edgetypes[i],0,2);
      }

      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i",
        show_immersed_shaded,show_immersed_shaded+1,show_immersed_shaded+2);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i",
        show_immersed_outlines,show_immersed_outlines+1,show_immersed_outlines+2);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i",
        show_immersed_points,show_immersed_points+1,show_immersed_points+2);

      for(i=0;i<MAX_CELL_TYPES;i++){
        show_immersed_shaded[i]   = CLAMP(show_immersed_shaded[i],0,1);
        show_immersed_outlines[i] = CLAMP(show_immersed_outlines[i],0,1);
        show_immersed_points[i]   = CLAMP(show_immersed_points[i],0,1);
      }
      continue;
    }
    if(Match(buffer, "NORTHANGLE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i", &vis_northangle);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %f %f %f", northangle_position, northangle_position + 1, northangle_position + 2);
      continue;
    }
    if(Match(buffer, "TREEPARMS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i", &mintreesize, &vis_xtree, &vis_ytree, &vis_ztree);
      mintreesize = MAX(mintreesize, 2);
      vis_xtree = CLAMP(vis_xtree, 0, 1);
      vis_ytree = CLAMP(vis_ytree, 0, 1);
      vis_ztree = CLAMP(vis_ztree, 0, 1);
      continue;
    }
    if(Match(buffer, "COLORBAR_SPLIT") == 1){
      int ii;

      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i %i %i ", colorsplit    , colorsplit + 1, colorsplit + 2, colorsplit + 3, colorsplit +  4, colorsplit +  5);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i %i %i ", colorsplit + 6, colorsplit + 7, colorsplit + 8, colorsplit + 9, colorsplit + 10, colorsplit + 11);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %f %f %f ", splitvals, splitvals + 1, splitvals + 2);

      for(ii=0;ii<12;ii++){
        colorsplit[ii] = CLAMP(colorsplit[ii],0,255);
      }
      continue;
    }
    if(Match(buffer, "SHOWGRAVVECTOR") == 1) {
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i", &showgravity_vector);
      continue;
    }
    if(Match(buffer, "ZAXISANGLES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %f %f %f", zaxis_angles, zaxis_angles + 1, zaxis_angles + 2);
      zaxis_angles_orig[0] = zaxis_angles[0];
      zaxis_angles_orig[1] = zaxis_angles[1];
      zaxis_angles_orig[2] = zaxis_angles[2];
      zaxis_custom = 1;
      update_zaxis_custom = 1;
      continue;
    }
    if(Match(buffer, "HISTOGRAM") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i %i %f",
      &histogram_static, &histogram_nbuckets, &histogram_show_numbers, &histogram_show_graph, &histogram_show_outline, &histogram_width_factor);
      continue;
    }
    if(Match(buffer, "GEOMDIAGS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i %i %i %i", &structured_isopen, &unstructured_isopen, &show_geometry_diagnostics,
        &highlight_edge0, &highlight_edge1, &highlight_edge2, &highlight_edgeother);
      continue;
    }
    if(Match(buffer, "GEOMSHOW") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i %i %i %f %f",
        &show_faces_interior, &show_faces_exterior, &show_faces_solid, &show_faces_outline, &smooth_geom_normal,
        &geom_force_transparent, &geom_transparency,&geom_outline_width);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i", &show_volumes_interior, &show_volumes_exterior, &show_volumes_solid, &show_volumes_outline);
      fgets(buffer, 255, stream);
      sscanf(buffer, " %f %f", &geom_vert_exag, &geom_max_angle);
      continue;
    }
    if(Match(buffer, "SHOWTRIANGLECOUNT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i", &show_triangle_count);
      continue;
    }
    if(Match(buffer, "SHOWDEVICEVALS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i %i %i %i %i %i %i %i %i",
        &showdevice_val, &showvdevice_val, &devicetypes_index, &colordevice_val, &vectortype, &viswindrose, &showdevice_type, &showdevice_unit,&showdevice_id);
      devicetypes_index = CLAMP(devicetypes_index, 0, ndevicetypes - 1);
      UpdateGluiDevices();
      continue;
    }
    if(Match(buffer, "SHOWMISSINGOBJECTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, " %i", &show_missing_objects);
      ONEORZERO(show_missing_objects);
      continue;
    }
    if(Match(buffer, "DEVICEVECTORDIMENSIONS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", &vector_baselength, &vector_basediameter, &vector_headlength, &vector_headdiameter);
      continue;
    }
    if(Match(buffer, "DEVICEBOUNDS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f", &device_valmin, &device_valmax);
      continue;
    }
    if(Match(buffer, "DEVICEORIENTATION") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f", &show_device_orientation, &orientation_scale);
      show_device_orientation = CLAMP(show_device_orientation, 0, 1);
      orientation_scale = CLAMP(orientation_scale, 0.1, 10.0);
      continue;
    }
    if(Match(buffer, "GVERSION") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &gversion);
      ONEORZERO(gversion);
    }
    if(Match(buffer, "GVECDOWN") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &gvec_down);
      ONEORZERO(gvec_down);
    }
    if(Match(buffer, "SCALEDFONT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i", &scaled_font2d_height, &scaled_font2d_height2width, &scaled_font2d_thickness);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i", &scaled_font3d_height, &scaled_font3d_height2width, &scaled_font3d_thickness);
    }
    if(Match(buffer, "FEDCOLORBAR") == 1){
      char *fbuff;

      fgets(buffer, 255, stream);
      TrimBack(buffer);
      fbuff = TrimFront(buffer);
      if(strlen(fbuff)>0)strcpy(default_fed_colorbar, fbuff);
    }
    if(Match(buffer, "FED") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &regenerate_fed);
      ONEORZERO(regenerate_fed);
      continue;
    }
    if(Match(buffer, "SHOWFEDAREA") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &show_fed_area);
      ONEORZERO(show_fed_area);
      continue;
    }
    if(Match(buffer, "USENEWDRAWFACE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &use_new_drawface);
      ONEORZERO(use_new_drawface);
      continue;
    }
    if(Match(buffer, "TLOAD") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f %i %i", &use_tload_begin, &tload_begin, &use_tload_end, &tload_end, &use_tload_skip, &tload_skip);
      continue;
    }
    if(Match(buffer, "VOLSMOKE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i",
        &glui_compress_volsmoke, &use_multi_threading, &load_at_rendertimes, &volbw, &show_volsmoke_moving);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f %f %f %f",
        &global_temp_min, &global_temp_cutoff, &global_temp_max, &fire_opacity_factor, &mass_extinct, &gpu_vol_factor, &nongpu_vol_factor);
      ONEORZERO(glui_compress_volsmoke);
      ONEORZERO(use_multi_threading);
      ONEORZERO(load_at_rendertimes);
      fire_opacity_factor = CLAMP(fire_opacity_factor, 1.0, 10.0);
      mass_extinct = CLAMP(mass_extinct, 100.0, 100000.0);
      InitVolRenderSurface(NOT_FIRSTCALL);
      continue;
    }
    if(Match(buffer, "WINDROSEMERGE")==1){
      float *xyzt;

      xyzt = windrose_merge_dxyzt;
      fgets(buffer, 255, stream);
      sscanf(buffer," %i %f %f %f %f",&windrose_merge_type,xyzt,xyzt+1,xyzt+2,xyzt+3);
      xyzt[0]=MAX(xyzt[0],0.0);
      xyzt[1]=MAX(xyzt[1],0.0);
      xyzt[2]=MAX(xyzt[2],0.0);
      xyzt[3]=MAX(xyzt[3],0.0);
    }
    if(Match(buffer, "WINDROSEDEVICE")==1){
      fgets(buffer, 255, stream);
      sscanf(buffer," %i %i %i %i %i %i %i %i %i",
        &viswindrose, &showref_windrose, &windrose_xy_vis, &windrose_xz_vis, &windrose_yz_vis, &windstate_windrose, &showlabels_windrose,
        &windrose_first,&windrose_next);
      viswindrose         = CLAMP(viswindrose, 0, 1);
      showref_windrose    = CLAMP(showref_windrose, 0, 1);
      windrose_xy_vis      = CLAMP(windrose_xy_vis, 0, 1);
      windrose_xz_vis      = CLAMP(windrose_xz_vis, 0, 1);
      windrose_yz_vis      = CLAMP(windrose_yz_vis, 0, 1);
      windstate_windrose  = CLAMP(windstate_windrose, 0, 1);
      showlabels_windrose = CLAMP(showlabels_windrose, 0, 1);
      if(windrose_first < 0)windrose_first = 0;
      if(windrose_next < 1)windrose_next = 1;

      fgets(buffer, 255, stream);
      sscanf(buffer," %i %i %i %f %i %i",    &nr_windrose, &ntheta_windrose, &scale_windrose, &radius_windrose, &scale_increment_windrose, &scale_max_windrose);
      nr_windrose              = ABS(nr_windrose);
      ntheta_windrose          = ABS(ntheta_windrose);
      radius_windrose          = ABS(radius_windrose);
      scale_windrose           = CLAMP(scale_windrose,0,1);
      scale_increment_windrose = CLAMP(scale_increment_windrose, 1, 50);
      scale_max_windrose       = CLAMP(scale_max_windrose, 0, 100);
      continue;
    }
    if(Match(buffer, "BOUNDARYTWOSIDE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &showpatch_both);
      ONEORZERO(showpatch_both);
    }
    if(Match(buffer, "MESHOFFSET") == 1){
      int meshnum;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &meshnum);
      if(meshnum >= 0 && meshnum<nmeshes){
        meshdata *meshi;

        meshi = meshinfo + meshnum;
        meshi->mesh_offset_ptr = meshi->mesh_offset;
      }
      continue;
    }
#ifdef pp_LANG
    if(Match(buffer, "STARTUPLANG") == 1){
      char *bufptr;

      fgets(buffer, 255, stream);
      TrimBack(buffer);
      bufptr = TrimFront(buffer);
      strncpy(startup_lang_code, bufptr, 2);
      startup_lang_code[2] = '\0';
      if(tr_name == NULL){
        int langlen;

        langlen = strlen(bufptr);
        NewMemory((void **)&tr_name, langlen + 48 + 1);
        strcpy(tr_name, bufptr);
      }
      continue;
    }
#endif
    if(Match(buffer, "MESHVIS") == 1){
      int nm;
      meshdata *meshi;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &nm);
      for(i = 0; i<nm; i++){
        if(i>nmeshes - 1)break;
        meshi = meshinfo + i;
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &meshi->blockvis);
        ONEORZERO(meshi->blockvis);
      }
      continue;
    }
    if(Match(buffer, "SPHERESEGS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &device_sphere_segments);
      device_sphere_segments = CLAMP(device_sphere_segments, 6, 48);
      InitSphere(device_sphere_segments, 2 * device_sphere_segments);
      InitCircle(2 * device_sphere_segments, &object_circ);
      continue;
    }
    if(Match(buffer, "SHOWEVACSLICES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i", &show_evac_slices, &constant_evac_coloring, &show_evac_colorbar);
      ONEORZERO(show_evac_slices);
      if(constant_evac_coloring != 1)constant_evac_coloring = 0;
      data_evac_coloring = 1 - constant_evac_coloring;
      ONEORZERO(show_evac_colorbar);
      UpdateSliceMenuShow();
      UpdateEvacParms();
      continue;
    }
    if(Match(buffer, "DIRECTIONCOLOR") == 1){
      float *dc;

      dc = direction_color;
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", dc, dc + 1, dc + 2);
      dc[3] = 1.0;
      direction_color_ptr = GetColorPtr(direction_color);
      UpdateSliceMenuShow();
      UpdateEvacParms();
      continue;
    }
    if(Match(buffer, "OFFSETSLICE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &offset_slice);
      ONEORZERO(offset_slice);
      continue;
    }
    if(Match(buffer, "VECLENGTH") == 1){
      float vf = 1.0;
      int dummy;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f", &dummy, &vf);
      vecfactor = vf;
      continue;
    }
    if(Match(buffer, "VECCONTOURS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &show_node_slices_and_vectors,&show_cell_slices_and_vectors);
      ONEORZERO(show_node_slices_and_vectors);
      ONEORZERO(show_cell_slices_and_vectors);
      continue;
    }
    if(Match(buffer, "ISOTRAN2") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &transparent_state);
      continue;
    }
    if(Match(buffer, "SHOWTETRAS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &show_volumes_solid, &show_volumes_outline);
      continue;
    }
    if(Match(buffer, "SHOWTRIANGLES") == 1){
      int dummy;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i %i", &show_iso_solid, &show_iso_outline, &show_iso_verts, &show_iso_normal, &dummy, &smooth_iso_normal);
      ONEORZERO(show_iso_solid);
      ONEORZERO(show_iso_outline);
      ONEORZERO(show_iso_verts);
      ONEORZERO(show_iso_normal);
      ONEORZERO(smooth_iso_normal);
#ifdef pp_BETA
      ONEORZERO(show_iso_normal);
#else
      show_iso_normal = 0;
#endif
      visAIso = show_iso_solid * 1 + show_iso_outline * 2 + show_iso_verts * 4;
      continue;
    }
    if(Match(buffer, "SHOWSTREAK") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i", &streak5show, &streak5step, &showstreakhead, &streak_index);
      ONEORZERO(streak5show);
      if(streak5show == 0)streak_index = -2;
      ONEORZERO(showstreakhead);
      update_streaks = 1;
      continue;
    }

    if(Match(buffer, "SHOWTERRAIN") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visTerrainType);
      continue;
    }
    if(Match(buffer, "STEREO") == 1){
      fgets(buffer, 255, stream);
      stereotypeOLD = stereotype;
      sscanf(buffer, "%i", &stereotype);
      stereotype = CLAMP(stereotype, 0, 5);
      if(stereotype == STEREO_TIME&&videoSTEREO != 1)stereotype = STEREO_NONE;
      UpdateGluiStereo();
      continue;
    }
    if(Match(buffer, "TERRAINPARMS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i", terrain_rgba_zmin, terrain_rgba_zmin + 1, terrain_rgba_zmin + 2);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i", terrain_rgba_zmax, terrain_rgba_zmax + 1, terrain_rgba_zmax + 2);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f", &vertical_factor);

      for(i = 0; i<3; i++){
        terrain_rgba_zmin[i] = CLAMP(terrain_rgba_zmin[i], 0, 2255);
        terrain_rgba_zmax[i] = CLAMP(terrain_rgba_zmax[i], 0, 2255);
      }
      vertical_factor = CLAMP(vertical_factor, 0.25, 4.0);
      UpdateTerrain(0, vertical_factor);
      UpdateTerrainColors();
      continue;
    }
    if(Match(buffer, "SMOKESENSORS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &show_smokesensors, &test_smokesensors);
      continue;
    }
#ifdef pp_GPU
    if(gpuactive == 1 && Match(buffer, "USEGPU") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &usegpu);
      ONEORZERO(usegpu);
      continue;
    }
#endif
    if(Match(buffer, "V_PLOT3D") == 1){
      int tempval;
      int n3d;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &tempval);
      if(tempval<0)tempval = 0;
      n3d = tempval;
      if(n3d>mxplot3dvars)n3d = mxplot3dvars;
      for(i = 0; i<n3d; i++){
        int iplot3d, isetmin, isetmax;
        float p3mintemp, p3maxtemp;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %i %f %i %f", &iplot3d, &isetmin, &p3mintemp, &isetmax, &p3maxtemp);
        iplot3d--;
        if(iplot3d >= 0 && iplot3d<mxplot3dvars){
          setp3min[iplot3d] = isetmin;
          setp3max[iplot3d] = isetmax;
          p3min[iplot3d] = p3mintemp;
          p3max[iplot3d] = p3maxtemp;
        }
      }
      continue;
    }
    if(Match(buffer, "CACHE_QDATA") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &cache_qdata);
      ONEORZERO(cache_qdata);
      continue;
    }
    if(Match(buffer, "UNLOAD_QDATA") == 1){
      int unload_qdata;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &unload_qdata);
      cache_qdata = 1 - unload_qdata;
      ONEORZERO(cache_qdata);
      continue;
    }
    if(Match(buffer, "CACHE_BOUNDARYDATA") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &cache_boundarydata);
      continue;
    }
    if(Match(buffer, "TREECOLORS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", trunccolor, trunccolor + 1, trunccolor + 2);
      sscanf(buffer, "%f %f %f", treecolor, treecolor + 1, treecolor + 2);
      sscanf(buffer, "%f %f %f", treecharcolor, treecharcolor + 1, treecharcolor + 2);
      for(i = 0; i<3; i++){
        treecolor[i] = CLAMP(treecolor[i], 0.0, 1.0);
        treecolor_uc[i] = 255 * treecolor[i];
        treecharcolor[i] = CLAMP(treecharcolor[i], 0.0, 1.0);
        treecharcolor_uc[i] = 255 * treecharcolor[i];
        trunccolor[i] = CLAMP(trunccolor[i], 0.0, 1.0);
        trunccolor_uc[i] = 255 * trunccolor[i];
      }
      treecolor[3] = 1.0;
      treecharcolor[3] = 1.0;
      trunccolor[3] = 1.0;
      treecolor_uc[3] = 255;
      treecharcolor_uc[3] = 255;
      trunccolor_uc[3] = 255;
      continue;
    }
    if(Match(buffer, "TRAINERVIEW") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &trainerview);
      if(trainerview != 2 && trainerview != 3)trainerview = 1;
      continue;
    }
    if(Match(buffer, "SHOWTRANSPARENTVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &show_transparent_vents);
      ONEORZERO(show_transparent_vents);
      continue;
    }
    if(Match(buffer, "COLORBARTYPE") == 1){
      char *label;

      update_colorbartype = 1;
      fgets(buffer, 255, stream);
      label = strchr(buffer, '%');
      if(label == NULL){
        sscanf(buffer, "%i", &colorbartype);
        RemapColorbarType(colorbartype, colorbarname);
      }
      else{
        label++;
        TrimBack(label);
        label = TrimFront(label);
        strcpy(colorbarname, label);
      }
      continue;
    }
    if(Match(buffer, "FIRECOLORMAP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &firecolormap_type, &fire_colorbar_index_ini);
      firecolormap_type_save = firecolormap_type;
      update_fire_colorbar_index = 1;
      continue;
    }
    if(Match(buffer, "SHOWEXTREMEDATA") == 1){
      int below = -1, above = -1, show_extremedata;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i", &show_extremedata, &below, &above);
      if(below == -1 && above == -1){
        if(below == -1)below = 0;
        if(below != 0)below = 1;
        if(above == -1)above = 0;
        if(above != 0)above = 1;
      }
      else{
        if(show_extremedata != 1)show_extremedata = 0;
        if(show_extremedata == 1){
          below = 1;
          above = 1;
        }
        else{
          below = 0;
          above = 0;
        }
      }
      show_extreme_mindata = below;
      show_extreme_maxdata = above;
      continue;
    }
    if(Match(buffer, "EXTREMECOLORS") == 1){
      int mmin[3], mmax[3];

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i %i",
        mmin, mmin + 1, mmin + 2,
        mmax, mmax + 1, mmax + 2);
      for(i = 0; i<3; i++){
        rgb_below_min[i] = CLAMP(mmin[i], 0, 255);
        rgb_above_max[i] = CLAMP(mmax[i], 0, 255);
      }
      continue;
    }
    if(Match(buffer, "SLICEAVERAGE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i", &slice_average_flag, &slice_average_interval, &vis_slice_average);
      ONEORZERO(slice_average_flag);
      if(slice_average_interval<0.0)slice_average_interval = 0.0;
      continue;
    }
    if(Match(buffer, "SKYBOX") == 1){
      skyboxdata *skyi;

      FreeSkybox();
      nskyboxinfo = 1;
      NewMemory((void **)&skyboxinfo, nskyboxinfo*sizeof(skyboxdata));
      skyi = skyboxinfo;

      for(i = 0; i<6; i++){
        fgets(buffer, 255, stream);
        LoadSkyTexture(buffer, skyi->face + i);
      }
    }
    if(Match(buffer, "C_PLOT3D") == 1){
      int tempval;
      int n3d;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &tempval);
      if(tempval<0)tempval = 0;
      n3d = tempval;
      if(n3d>mxplot3dvars)n3d = mxplot3dvars;
      for(i = 0; i<n3d; i++){
        int iplot3d, isetmin, isetmax;
        float p3mintemp, p3maxtemp;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %i %f %i %f", &iplot3d, &isetmin, &p3mintemp, &isetmax, &p3maxtemp);
        iplot3d--;
        if(iplot3d >= 0 && iplot3d<mxplot3dvars){
          setp3chopmin[iplot3d] = isetmin;
          setp3chopmax[iplot3d] = isetmax;
          p3chopmin[iplot3d] = p3mintemp;
          p3chopmax[iplot3d] = p3maxtemp;
        }
      }
      continue;
    }
    if(Match(buffer, "DEVICENORMLENGTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f", &devicenorm_length);
      if(devicenorm_length<0.0 || devicenorm_length>1.0)devicenorm_length = 0.1;
      continue;
    }
    if(Match(buffer, "SHOWHRRLABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visHRRlabel);
      ONEORZERO(visHRRlabel);
      UpdateHrrinfo(visHRRlabel);
      continue;
    }
    if(Match(buffer, "SHOWHRRCUTOFF") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &show_hrrcutoff);
      ONEORZERO(show_hrrcutoff);
      continue;
    }
    if(Match(buffer, "TWOSIDEDVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &show_bothsides_int, &show_bothsides_ext);
      ONEORZERO(show_bothsides_int);
      ONEORZERO(show_bothsides_ext);
      continue;
    }
    if(Match(buffer, "SHOWSLICEINOBST") == 1){
      if((localfile==0&&solid_ht3d==0)||localfile==1){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &show_slice_in_obst);
        show_slice_in_obst=CLAMP(show_slice_in_obst,0,2);
      }
      continue;
    }
    if(Match(buffer, "SKIPEMBEDSLICE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &skip_slice_in_embedded_mesh);
      ONEORZERO(skip_slice_in_embedded_mesh);
      continue;
    }
    if(Match(buffer, "PERCENTILELEVEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f", &percentile_level);
      if(percentile_level<0.0)percentile_level = 0.01;
      if(percentile_level>0.5)percentile_level = 0.01;
      continue;
    }
    if(Match(buffer, "TRAINERMODE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &trainer_mode);
      continue;
    }
    if(Match(buffer, "COMPRESSAUTO") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &compress_autoloaded);
      continue;
    }
    if(Match(buffer, "PLOT3DAUTO") == 1){
      int n3dsmokes = 0;
      int seq_id;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &n3dsmokes);
        for(i = 0; i<n3dsmokes; i++){
          fgets(buffer, 255, stream);
          sscanf(buffer, "%i", &seq_id);
          GetStartupPlot3d(seq_id);
        }
        update_load_files = 1;
        continue;
      }
      if(Match(buffer, "VSLICEAUTO") == 1){
        int n3dsmokes = 0;
        int seq_id;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &n3dsmokes);
        for(i = 0; i<n3dsmokes; i++){
          fgets(buffer, 255, stream);
          sscanf(buffer, "%i", &seq_id);
          GetStartupVSlice(seq_id);
        }
        update_load_files = 1;
        continue;
      }
      if(Match(buffer, "SLICEAUTO") == 1){
        int n3dsmokes = 0;
        int seq_id;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &n3dsmokes);
        for(i = 0; i<n3dsmokes; i++){
          fgets(buffer, 255, stream);
          sscanf(buffer, "%i", &seq_id);
          GetStartupSlice(seq_id);
        }
        update_load_files = 1;
        continue;
      }
    if(Match(buffer, "MSLICEAUTO") == 1){
      int n3dsmokes = 0;
      int seq_id;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &n3dsmokes);
      for(i = 0; i<n3dsmokes; i++){

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &seq_id);

        if(seq_id >= 0 && seq_id<nmultisliceinfo){
          multislicedata *mslicei;

          mslicei = multisliceinfo + seq_id;
          mslicei->autoload = 1;
        }
      }
      update_load_files = 1;
      continue;
    }
    if(Match(buffer, "PARTAUTO") == 1){
      int n3dsmokes = 0;
      int seq_id;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &n3dsmokes);
      for(i = 0; i<n3dsmokes; i++){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &seq_id);
        GetStartupPart(seq_id);
      }
      update_load_files = 1;
      continue;
    }
    if(Match(buffer, "ISOAUTO") == 1){
      int n3dsmokes = 0;
      int seq_id;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &n3dsmokes);
      for(i = 0; i<n3dsmokes; i++){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &seq_id);
        GetStartupISO(seq_id);
      }
      update_load_files = 1;
      continue;
    }
    if(Match(buffer, "S3DAUTO") == 1){
      int n3dsmokes = 0;
      int seq_id;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &n3dsmokes);
      for(i = 0; i<n3dsmokes; i++){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &seq_id);
        GetStartupSmoke(seq_id);
      }
      update_load_files = 1;
      continue;
    }
    if(Match(buffer, "PATCHAUTO") == 1){
      int n3dsmokes = 0;
      int seq_id;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &n3dsmokes);
      for(i = 0; i<n3dsmokes; i++){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &seq_id);
        GetStartupBoundary(seq_id);
      }
      update_load_files = 1;
      continue;
    }
    if(Match(buffer, "LOADFILESATSTARTUP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &loadfiles_at_startup);
      continue;
    }
    if(Match(buffer, "SHOWALLTEXTURES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &showall_textures);
      continue;
    }
    if(Match(buffer, "ENABLETEXTURELIGHTING") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &enable_texture_lighting);
      continue;
    }
    if(Match(buffer, "PROJECTION") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &projection_type);
      SceneMotionCB(PROJECTION);
      UpdateProjectionType();
      continue;
    }
    if(Match(buffer, "V_PARTICLES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f", &setpartmin, &partmin, &setpartmax, &partmax);
      continue;
    }
    if(Match(buffer, "V5_PARTICLES") == 1){
      int ivmin, ivmax;
      float vmin, vmax;
      char short_label[256], *s1;

      strcpy(short_label, "");
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f %s", &ivmin, &vmin, &ivmax, &vmax, short_label);

#define MAXVAL 100000000.0
#define MINVAL -100000000.0

      if(vmax > MAXVAL)vmax = 1.0;
      if(vmin < MINVAL)vmin = 0.0;

      if(npart5prop>0){
        int label_index = 0;

        TrimBack(short_label);
        s1 = TrimFront(short_label);
        if(strlen(s1)>0)label_index = GetPartPropIndexS(s1);
        if(label_index >= 0 && label_index<npart5prop){
          partpropdata *propi;

          propi = part5propinfo + label_index;
          propi->setvalmin = ivmin;
          propi->setvalmax = ivmax;
          propi->valmin = vmin;
          propi->valmax = vmax;
          switch(ivmin){
          case PERCENTILE_MIN:
            propi->percentile_min = vmin;
            break;
          case GLOBAL_MIN:
            propi->global_min = vmin;
            break;
          case SET_MIN:
            propi->user_min = vmin;
            break;
          default:
            ASSERT(FFALSE);
            break;
          }
          switch(ivmax){
          case PERCENTILE_MAX:
            propi->percentile_max = vmax;
            break;
          case GLOBAL_MAX:
            propi->global_max = vmax;
            break;
          case SET_MAX:
            propi->user_max = vmax;
            break;
          default:
            ASSERT(FFALSE);
            break;
          }
        }
      }
      continue;
    }
    if(Match(buffer, "C_PARTICLES") == 1){
      int icmin, icmax;
      float cmin, cmax;
      char short_label[256], *s1;

      strcpy(short_label, "");
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f %s", &icmin, &cmin, &icmax, &cmax, short_label);

      if(npart5prop>0){
        int label_index = 0;

        TrimBack(short_label);
        s1 = TrimFront(short_label);
        if(strlen(s1)>0)label_index = GetPartPropIndexS(s1);
        if(label_index >= 0 && label_index<npart5prop){
          partpropdata *propi;

          propi = part5propinfo + label_index;
          propi->setchopmin = icmin;
          propi->setchopmax = icmax;
          propi->chopmin = cmin;
          propi->chopmax = cmax;
        }
      }
      continue;
    }
    if(Match(buffer, "V_SLICE") == 1){
      float valmin, valmax;
      int setvalmin, setvalmax;
      char *level_val;

      fgets(buffer, 255, stream);
      strcpy(buffer2, "");
      sscanf(buffer, "%i %f %i %f %s", &setvalmin, &valmin, &setvalmax, &valmax, buffer2);
      {
        char *colon;

        colon = strstr(buffer, ":");
        level_val = NULL;
        if(colon != NULL){
          level_val = colon + 1;
          TrimBack(level_val);
          *colon = 0;
          if(strlen(level_val)>1){
            sscanf(level_val, "%f %f %i", &slice_line_contour_min, &slice_line_contour_max, &slice_line_contour_num);
          }
          {
            level_val = NULL;
          }
        }
      }
      if(strcmp(buffer2, "") != 0){
        TrimBack(buffer2);
        for(i = 0; i<nslice_type; i++){
          if(strcmp(slicebounds[i].datalabel, buffer2) != 0)continue;
          slicebounds[i].setvalmin = setvalmin;
          slicebounds[i].setvalmax = setvalmax;
          slicebounds[i].valmin = valmin;
          slicebounds[i].valmax = valmax;
          if(level_val != NULL){
            slicebounds[i].line_contour_min = slice_line_contour_min;
            slicebounds[i].line_contour_max = slice_line_contour_max;
            slicebounds[i].line_contour_num = slice_line_contour_num;
          }
          break;
        }
      }
      else{
        for(i = 0; i<nslice_type; i++){
          slicebounds[i].setvalmin = setvalmin;
          slicebounds[i].setvalmax = setvalmax;
          slicebounds[i].valmin = valmin;
          slicebounds[i].valmax = valmax;
          slicebounds[i].line_contour_min = slice_line_contour_min;
          slicebounds[i].line_contour_max = slice_line_contour_max;
          slicebounds[i].line_contour_num = slice_line_contour_num;
        }
      }
      continue;
    }
    if(Match(buffer, "C_SLICE") == 1){
      float valmin, valmax;
      int setvalmin, setvalmax;

      fgets(buffer, 255, stream);
      strcpy(buffer2, "");
      sscanf(buffer, "%i %f %i %f %s", &setvalmin, &valmin, &setvalmax, &valmax, buffer2);
      if(strcmp(buffer, "") != 0){
        for(i = 0; i<nslice_type; i++){
          if(strcmp(slicebounds[i].datalabel, buffer2) != 0)continue;
          slicebounds[i].setchopmin = setvalmin;
          slicebounds[i].setchopmax = setvalmax;
          slicebounds[i].chopmin = valmin;
          slicebounds[i].chopmax = valmax;
          break;
        }
      }
      else{
        for(i = 0; i<nslice_type; i++){
          slicebounds[i].setchopmin = setvalmin;
          slicebounds[i].setchopmax = setvalmax;
          slicebounds[i].chopmin = valmin;
          slicebounds[i].chopmax = valmax;
        }
      }
      continue;
    }
    if(Match(buffer, "V_ISO") == 1){
      float valmin, valmax;
      int setvalmin, setvalmax;

      fgets(buffer, 255, stream);
      strcpy(buffer2, "");
      sscanf(buffer, "%i %f %i %f %s", &setvalmin, &valmin, &setvalmax, &valmax, buffer2);
      if(strcmp(buffer2, "") != 0){
        for(i = 0; i<niso_bounds; i++){
          if(strcmp(isobounds[i].datalabel, buffer2) != 0)continue;
          isobounds[i].setvalmin = setvalmin;
          isobounds[i].setvalmax = setvalmax;
          isobounds[i].valmin = valmin;
          isobounds[i].valmax = valmax;
          break;
        }
      }
      else{
        for(i = 0; i<niso_bounds; i++){
          isobounds[i].setvalmin = setvalmin;
          isobounds[i].setvalmax = setvalmax;
          isobounds[i].valmin = valmin;
          isobounds[i].valmax = valmax;
        }
      }
      continue;
    }
    if(Match(buffer, "C_ISO") == 1){
      float valmin, valmax;
      int setvalmin, setvalmax;

      fgets(buffer, 255, stream);
      strcpy(buffer2, "");
      sscanf(buffer, "%i %f %i %f %s", &setvalmin, &valmin, &setvalmax, &valmax, buffer2);
      if(strcmp(buffer, "") != 0){
        for(i = 0; i<niso_bounds; i++){
          if(strcmp(isobounds[i].datalabel, buffer2) != 0)continue;
          isobounds[i].setchopmin = setvalmin;
          isobounds[i].setchopmax = setvalmax;
          isobounds[i].chopmin = valmin;
          isobounds[i].chopmax = valmax;
          break;
        }
      }
      else{
        for(i = 0; i<niso_bounds; i++){
          isobounds[i].setchopmin = setvalmin;
          isobounds[i].setchopmax = setvalmax;
          isobounds[i].chopmin = valmin;
          isobounds[i].chopmax = valmax;
        }
      }
      continue;
    }
    if(Match(buffer, "V_BOUNDARY") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f %s", &setpatchmin, &patchmin, &setpatchmax, &patchmax, buffer2);
      if(strcmp(buffer2, "") != 0)Local2GlobalBoundaryBounds(buffer2);
      continue;
    }
    if(Match(buffer, "C_BOUNDARY") == 1){
      float valmin, valmax;
      int setvalmin, setvalmax;
      char *buffer2ptr;
      int lenbuffer2;

      fgets(buffer, 255, stream);
      strcpy(buffer2, "");
      sscanf(buffer, "%i %f %i %f %s", &setvalmin, &valmin, &setvalmax, &valmax, buffer2);
      TrimBack(buffer2);
      buffer2ptr = TrimFront(buffer2);
      lenbuffer2 = strlen(buffer2ptr);
      for(i = 0; i<npatchinfo; i++){
        patchdata *patchi;

        patchi = patchinfo + i;
        if(lenbuffer2 == 0 || strcmp(patchi->label.shortlabel, buffer2ptr) == 0){
          patchi->chopmin = valmin;
          patchi->chopmax = valmax;
          patchi->setchopmin = setvalmin;
          patchi->setchopmax = setvalmax;
        }
      }
      UpdateBoundaryListIndex2(buffer2ptr);
      continue;
    }
    if(Match(buffer, "V_ZONE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f", &setzonemin, &zoneusermin, &setzonemax, &zoneusermax);
      if(setzonemin == PERCENTILE_MIN)setzonemin = GLOBAL_MIN;
      if(setzonemax == PERCENTILE_MIN)setzonemax = GLOBAL_MIN;
      if(setzonemin == SET_MIN)zonemin = zoneusermin;
      if(setzonemax == SET_MAX)zonemax = zoneusermax;
      UpdateGluiZoneBounds();
      continue;
    }
    if(Match(buffer, "V_TARGET") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %f %s", &settargetmin, &targetmin, &settargetmax, &targetmax, buffer2);
      continue;
    }
    if(Match(buffer, "ZONEVIEW") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f", &zone_hvac_diam);
      if(zone_hvac_diam < 0.0)zone_hvac_diam = 0.0;
      continue;
    }
    if(Match(buffer, "OUTLINEMODE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &highlight_flag, &outline_color_flag);
      if(nmeshes<2){
        ONEORZERO(highlight_flag);
      }
      continue;
    }
    if(Match(buffer, "SLICEDATAOUT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &output_slicedata);
      ONEORZERO(output_slicedata);
      continue;
    }
    if(Match(buffer, "PATCHDATAOUT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %f %f %f %f %f %f %f", &output_patchdata,
        &patchout_tmin, &patchout_tmax,
        &patchout_xmin, &patchout_xmax,
        &patchout_ymin, &patchout_ymax,
        &patchout_zmin, &patchout_zmax
        );
      ONEORZERO(output_patchdata);
      continue;
    }
    if(Match(buffer, "SMOKE3DZIPSTEP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &smoke3dzipstep);
      if(smoke3dzipstep<1)smoke3dzipstep = 1;
      smoke3dzipskip = smoke3dzipstep - 1;
      continue;
    }
    if(Match(buffer, "SLICEZIPSTEP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &slicezipstep);
      if(slicezipstep<1)slicezipstep = 1;
      slicezipskip = slicezipstep - 1;
      continue;
    }
    if(Match(buffer, "ISOZIPSTEP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &isozipstep);
      if(isozipstep<1)isozipstep = 1;
      isozipskip = isozipstep - 1;
      continue;
    }
    if(Match(buffer, "BOUNDZIPSTEP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &boundzipstep);
      if(boundzipstep<1)boundzipstep = 1;
      boundzipskip = boundzipstep - 1;
      continue;
    }
    if(Match(buffer, "LOADINC") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &load_incremental);
      continue;
    }
    if(Match(buffer, "MSCALE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", mscale, mscale + 1, mscale + 2);
      continue;
    }
    if(Match(buffer, "RENDERCLIP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i",
        &clip_rendered_scene, &render_clip_left, &render_clip_right, &render_clip_bottom, &render_clip_top);
      continue;
    }
    if(Match(buffer, "CLIP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f", &nearclip, &farclip);
      continue;
    }
    if(Match(buffer, "SHOWTRACERSALWAYS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &show_tracers_always);
      ONEORZERO(show_tracers_always);
      continue;
    }
    if(Match(buffer, "PART5COLOR") == 1){
      for(i = 0; i<npart5prop; i++){
        partpropdata *propi;

        propi = part5propinfo + i;
        propi->display = 0;
      }
      part5colorindex = 0;
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &i);
      if(i >= 0 && i<npart5prop){
        partpropdata *propi;

        part5colorindex = i;
        propi = part5propinfo + i;
        propi->display = 1;
      }
      continue;
    }
    if(Match(buffer, "PART5PROPDISP") == 1){
      char *token;

      for(i = 0; i<npart5prop; i++){
        partpropdata *propi;
        int j;

        propi = part5propinfo + i;
        fgets(buffer, 255, stream);

        TrimBack(buffer);
        token = strtok(buffer, " ");
        j = 0;
        while(token != NULL&&j<npartclassinfo){
          int visval;

          sscanf(token, "%i", &visval);
          propi->class_vis[j] = visval;
          token = strtok(NULL, " ");
          j++;
        }
      }
      CheckMemory;
      continue;
    }
    if(Match(buffer, "COLORBAR") == 1){
      float *rgb_ini_copy;
      int nn;

      CheckMemory;
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i", &nrgb_ini, &usetexturebar, &colorbar_select_index, &colorband);
      colorband = CLAMP(colorband, 1, 10);
      FREEMEMORY(rgb_ini);
      if(NewMemory((void **)&rgb_ini, 4 * nrgb_ini*sizeof(float)) == 0)return 2;
      rgb_ini_copy = rgb_ini;
      for(nn = 0; nn<nrgb_ini; nn++){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%f %f %f ", rgb_ini_copy, rgb_ini_copy + 1, rgb_ini_copy + 2);
        rgb_ini_copy += 3;
      }
      InitRGB();
      if(colorbar_select_index >= 0 && colorbar_select_index <= 255){
        update_colorbar_select_index = 1;
      }
      continue;
    }
    if(Match(buffer, "COLOR2BAR") == 1){
      float *rgb_ini_copy;
      int nn;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &nrgb2_ini);
      if(nrgb2_ini<8){
        fprintf(stderr, "*** Error: must have at lease 8 colors in COLOR2BAR\n");
        exit(1);
      }
      FREEMEMORY(rgb2_ini);
      if(NewMemory((void **)&rgb2_ini, 4 * nrgb_ini*sizeof(float)) == 0)return 2;
      rgb_ini_copy = rgb2_ini;
      for(nn = 0; nn<nrgb2_ini; nn++){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%f %f %f ", rgb_ini_copy, rgb_ini_copy + 1, rgb_ini_copy + 2);
        rgb_ini_copy += 3;
      }
      continue;
    }
    if(Match(buffer, "P3DSURFACETYPE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &p3dsurfacetype);
      continue;
    }
    if(Match(buffer, "P3DSURFACESMOOTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &p3dsurfacesmooth);
      continue;
    }
    if(Match(buffer, "CULLFACES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &cullfaces);
      continue;
    }
    if(Match(buffer, "PARTPOINTSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &partpointsize);
      continue;
    }
    if(Match(buffer, "ISOPOINTSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &isopointsize);
      continue;
    }
    if(Match(buffer, "ISOLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &isolinewidth);
      continue;
    }
    if(Match(buffer, "PLOT3DPOINTSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &plot3dpointsize);
      continue;
    }
    if(Match(buffer, "GRIDLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &gridlinewidth);
      continue;
    }
    if(Match(buffer, "TICKLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &ticklinewidth);
      continue;
    }
    if(Match(buffer, "PLOT3DLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &plot3dlinewidth);
      continue;
    }
    if(Match(buffer, "VECTORCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &color_vector_black);
      continue;
    }
    if(Match(buffer, "VECTORPOINTSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &vectorpointsize);
      continue;
    }
    if(Match(buffer, "VECTORLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f", &vectorlinewidth, &slice_line_contour_width);
      continue;
    }
    if(Match(buffer, "STREAKLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &streaklinewidth);
      continue;
    }
    if(Match(buffer, "LINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &linewidth);
      solidlinewidth = linewidth;
      continue;
    }
    if(Match(buffer, "VENTLINEWIDTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &ventlinewidth);
      continue;
    }
    if(Match(buffer, "SLICEOFFSET") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &sliceoffset_factor);
      continue;
    }
    if(Match(buffer, "TITLESAFE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &titlesafe_offset);
      continue;
    }
    if(Match(buffer, "VENTOFFSET") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &ventoffset_factor);
      continue;
    }
    if(Match(buffer, "AXISSMOOTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &axislabels_smooth);
      continue;
    }
    if(Match(buffer, "SHOWBLOCKS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visBlocks);
      continue;
    }
    if(Match(buffer, "SHOWSENSORS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i ", &visSensor, &visSensorNorm);
      ONEORZERO(visSensor);
      ONEORZERO(visSensorNorm);
      continue;
    }
    if(Match(buffer, "AVATAREVAC") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &iavatar_evac);
      continue;
    }
    if(Match(buffer, "SHOWVENTFLOW") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i", &visVentHFlow, &visventslab, &visventprofile, &visVentVFlow, &visVentMFlow);
      continue;
    }
    if(Match(buffer, "SHOWVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visVents);
      continue;
    }
    if(Match(buffer, "SHOWROOMS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visCompartments);
      continue;
    }
    if(Match(buffer, "SHOWTIMELABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visTimelabel);
      continue;
    }
    if(Match(buffer, "SHOWHMSTIMELABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &vishmsTimelabel);
      continue;
    }
    if(Match(buffer, "SHOWFRAMELABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visFramelabel);
      continue;
    }
    if(Match(buffer, "SHOWHRRLABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visHRRlabel);
      continue;
    }
    if(Match(buffer, "RENDERFILETYPE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &render_filetype, &movie_filetype);
      continue;
    }
    if(Match(buffer, "MOVIEFILETYPE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i", &movie_filetype,&movie_framerate,&movie_bitrate,&quicktime_compatibility);
      continue;
    }
    if(Match(buffer, "RENDERFILELABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &render_label_type);
      ONEORZERO(render_label_type);
      continue;
    }
    if(Match(buffer, "CELLCENTERTEXT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &cell_center_text);
      continue;
    }
    if(Match(buffer, "SHOWGRIDLOC") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visgridloc);
      continue;
    }
    if(Match(buffer, "SHOWGRID") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visGrid);
      continue;
    }
    if(Match(buffer, "SHOWFLOOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visFloor);
      continue;
    }
    if(Match(buffer, "SPEED") == 1){
      fgets(buffer, 255, stream);
      //  sscanf(buffer,"%f %f",&speed_crawl,&speed_walk);
      continue;
    }
    if(Match(buffer, "FONTSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &fontindex);
      fontindex = CLAMP(fontindex, 0, SCALED_FONT);
      FontMenu(fontindex);
      continue;
    }
    if(Match(buffer, "ZOOM") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f ", &zoomindex, &zoom);
      if(zoomindex != -1){
        if(zoomindex<0)zoomindex = 2;
        if(zoomindex>4)zoomindex = 2;
        zoom = zooms[zoomindex];
      }
      else{
        if(zoom<zooms[0]){
          zoom = zooms[0];
          zoomindex = 0;
        }
        if(zoom>zooms[4]){
          zoom = zooms[4];
          zoomindex = 4;
        }
      }
      zoomini = zoom;
      updatezoomini = 1;
      ZoomMenu(zoomindex);
      continue;
    }
    if(Match(buffer, "APERATURE") == 1 || Match(buffer, "APERTURE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &apertureindex);
      apertureindex = CLAMP(apertureindex, 0, 4);
      ApertureMenu(apertureindex);
      continue;
    }
    if(Match(buffer, "SHOWWALLS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visWalls);
      continue;
    }
    if(Match(buffer, "SHOWCEILING") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visCeiling);
      continue;
    }
    if(Match(buffer, "SHOWTITLE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visTitle);
      continue;
    }
    if(Match(buffer, "SHOWCHID") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visCHID);
      continue;
    }
    if(Match(buffer, "SHOWTRANSPARENT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &visTransparentBlockage);
      continue;
    }
    if(Match(buffer, "SHOWCADOPAQUE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &viscadopaque);
      continue;
    }
    if(Match(buffer, "VECTORPOINTSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &vectorpointsize);
      continue;
    }
    if(Match(buffer, "VECTORSKIP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &vectorskip);
      if(vectorskip<1)vectorskip = 1;
      continue;
    }
    if(Match(buffer, "SPRINKLERABSSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &sprinklerabssize);
      continue;
    }
    if(Match(buffer, "SENSORABSSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &sensorabssize);
      continue;
    }
    if(Match(buffer, "SENSORRELSIZE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f ", &sensorrelsize);
      if(sensorrelsize<sensorrelsizeMIN)sensorrelsize = sensorrelsizeMIN;
      continue;
    }
    if(Match(buffer, "SETBW") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &setbw, &setbwdata);
      continue;
    }
    if(Match(buffer, "FLIP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &background_flip);
      continue;
    }
    if(Match(buffer, "COLORBAR_FLIP") == 1 || Match(buffer, "COLORBARFLIP") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &colorbar_flip,&colorbar_autoflip);
      continue;
    }
    if(Match(buffer, "TRANSPARENT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f", &use_transparency_data, &transparent_level);
      continue;
    }
    if(Match(buffer, "VENTCOLOR") == 1){
      float ventcolor_temp[4];

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", ventcolor_temp, ventcolor_temp + 1, ventcolor_temp + 2);
      ventcolor_temp[3] = 1.0;
      ventcolor = GetColorPtr(ventcolor_temp);
      updatefaces = 1;
      updateindexcolors = 1;
      continue;
    }
    if(Match(buffer, "STATICPARTCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", static_color, static_color + 1, static_color + 2);
      continue;
    }
    if(Match(buffer, "HEATOFFCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", heatoffcolor, heatoffcolor + 1, heatoffcolor + 2);
      continue;
    }
    if(Match(buffer, "HEATONCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", heatoncolor, heatoncolor + 1, heatoncolor + 2);
      continue;
    }
    if(Match(buffer, "SENSORCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", sensorcolor, sensorcolor + 1, sensorcolor + 2);
      continue;
    }
    if(Match(buffer, "SENSORNORMCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", sensornormcolor, sensornormcolor + 1, sensornormcolor + 2);
      continue;
    }
    if(Match(buffer, "SPRINKONCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", sprinkoncolor, sprinkoncolor + 1, sprinkoncolor + 2);
      continue;
    }
    if(Match(buffer, "SPRINKOFFCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", sprinkoffcolor, sprinkoffcolor + 1, sprinkoffcolor + 2);
      continue;
    }
    if(Match(buffer, "BACKGROUNDCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", backgroundbasecolor, backgroundbasecolor + 1, backgroundbasecolor + 2);
      SetColorControls();
      continue;
    }
    if(Match(buffer, "FOREGROUNDCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", foregroundbasecolor, foregroundbasecolor + 1, foregroundbasecolor + 2);
      SetColorControls();
      continue;
    }
    if(Match(buffer, "BLOCKCOLOR") == 1){
      float blockcolor_temp[4];

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", blockcolor_temp, blockcolor_temp + 1, blockcolor_temp + 2);
      blockcolor_temp[3] = 1.0;
      block_ambient2 = GetColorPtr(blockcolor_temp);
      updatefaces = 1;
      updateindexcolors = 1;
      continue;
    }
    if(Match(buffer, "BLOCKLOCATION") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &blocklocation);
      continue;
    }
    if(Match(buffer, "BEAM") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %f %i %i %i %i",
          &showbeam_as_line,&beam_line_width,&use_beamcolor,beam_color,beam_color+1,beam_color+2);
      showbeam_as_line = CLAMP(showbeam_as_line,0,1);
      use_beamcolor = CLAMP(use_beamcolor,0,1);
      beam_color[0] = CLAMP(beam_color[0], 0, 255);
      beam_color[1] = CLAMP(beam_color[1], 0, 255);
      beam_color[2] = CLAMP(beam_color[2], 0, 255);
      continue;
    }

    if(Match(buffer, "BLOCKSHININESS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f", &block_shininess);
      updatefaces = 1;
      updateindexcolors = 1;
      continue;
    }
    if(Match(buffer, "BLOCKSPECULAR") == 1){
      float blockspec_temp[4];

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", blockspec_temp, blockspec_temp + 1, blockspec_temp + 2);
      blockspec_temp[3] = 1.0;
      block_specular2 = GetColorPtr(blockspec_temp);
      updatefaces = 1;
      updateindexcolors = 1;
      continue;
    }
    if(Match(buffer, "SHOWOPENVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &visOpenVents, &visOpenVentsAsOutline);
      continue;
    }
    if(Match(buffer, "SHOWDUMMYVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visDummyVents);
      continue;
    }
    if(Match(buffer, "SHOWOTHERVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visOtherVents);
      ONEORZERO(visOtherVents);
      continue;
    }
    if(Match(buffer, "SHOWCVENTS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &visCircularVents, &circle_outline);
      visCircularVents = CLAMP(visCircularVents, 0, 2);
      circle_outline = CLAMP(circle_outline, 0, 1);
      continue;
    }
    if(Match(buffer, "SHOWTICKS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visFDSticks);
      continue;
    }
    if(Match(buffer, "USERTICKS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i %i %f", &visUSERticks, &auto_user_tick_placement, &user_tick_sub,
        &user_tick_show_x, &user_tick_show_y, &user_tick_show_z, &user_tick_direction);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", user_tick_origin, user_tick_origin + 1, user_tick_origin + 2);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", user_tick_min, user_tick_min + 1, user_tick_min + 2);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", user_tick_max, user_tick_max + 1, user_tick_max + 2);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", user_tick_step, user_tick_step + 1, user_tick_step + 2);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i", &user_tick_show_x, &user_tick_show_y, &user_tick_show_z);
      continue;
    }
    if(Match(buffer, "SHOWLABELS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visLabels);
      continue;
    }
    if(Match(buffer, "BOUNDCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", boundcolor, boundcolor + 1, boundcolor + 2);
      continue;
    }
    if(Match(buffer, "TIMEBARCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", timebarcolor, timebarcolor + 1, timebarcolor + 2);
      continue;
    }
    if(Match(buffer, "CONTOURTYPE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &contour_type);
      contour_type = CLAMP(contour_type, 0, 2);
      continue;
    }
    if(Match(buffer, "P3CONT3DSMOOTH") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &p3cont3dsmooth);
      continue;
    }
    if(Match(buffer, "SURFINC") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &surfincrement);
      continue;
    }
    if(Match(buffer, "FRAMERATE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visFramerate);
      continue;
    }
    if(Match(buffer, "SHOWFRAMERATE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visFramerate);
      continue;
    }
    if(Match(buffer, "SHOWFRAME") == 1 && Match(buffer, "SHOWFRAMERATE") != 1 && Match(buffer, "SHOWFRAMELABEL") != 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visFrame);
      ONEORZERO(visFrame);
      continue;
    }
    if(Match(buffer, "FRAMERATEVALUE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &frameratevalue);
      FrameRateMenu(frameratevalue);
      continue;
    }
    if(Match(buffer, "SHOWSPRINKPART") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visSprinkPart);
      continue;
    }
    if(Match(buffer, "SHOWAXISLABELS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visaxislabels);
      continue;
    }
#ifdef pp_memstatus
    if(Match(buffer, "SHOWMEMLOAD") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visAvailmemory);
      continue;
    }
#endif
    if(Match(buffer, "SHOWBLOCKLABEL") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visMeshlabel);
      continue;
    }
    if(Match(buffer, "SHOWVZONE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &visVZone, &show_zonelower);
      continue;
    }
    if(Match(buffer, "SHOWZONEFIRE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i ", &viszonefire);
      if(viszonefire != 0)viszonefire = 1;
      continue;
    }
    if(Match(buffer, "SHOWSZONE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visSZone);
      continue;
    }
    if(Match(buffer, "SHOWHZONE") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visHZone);
      continue;
    }
    if(Match(buffer, "SHOWHAZARDCOLORS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &zonecolortype);
      zonecolortype = CLAMP(zonecolortype, 0, 2);
      continue;
    }
    if(Match(buffer, "SHOWSMOKEPART") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visSmokePart);
      continue;
    }
    if(Match(buffer, "RENDEROPTION") == 1){
      int nheight360_temp = 0;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i", &render_window_size, &resolution_multiplier, &nheight360_temp);
      if(nheight360_temp > 0){
        nheight360 = nheight360_temp;
        nwidth360 = 2 * nheight360;
      }
      RenderMenu(render_window_size);
      continue;
    }
    if(Match(buffer, "SHOWISONORMALS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &show_iso_normal);
      if(show_iso_normal != 1)show_iso_normal = 0;
      continue;
    }
    if(Match(buffer, "SHOWISO") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visAIso);
      visAIso &= 7;
      show_iso_solid = (visAIso & 1) / 1;
      show_iso_outline = (visAIso & 2) / 2;
      show_iso_verts = (visAIso & 4) / 4;
      continue;
    }
    if(trainer_mode == 0 && windowresized == 0){
      if(Match(buffer, "WINDOWWIDTH") == 1){
        int scrWidth;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &scrWidth);
        if(scrWidth <= 0){
          scrWidth = glutGet(GLUT_SCREEN_WIDTH);
        }
        if(scrWidth != screenWidth){
          SetScreenSize(&scrWidth, NULL);
          screenWidthINI = scrWidth;
          update_screensize = 1;
        }
        continue;
      }
      if(Match(buffer, "WINDOWHEIGHT") == 1){
        int scrHeight;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &scrHeight);
        if(scrHeight <= 0){
          scrHeight = glutGet(GLUT_SCREEN_HEIGHT);
        }
        if(scrHeight != screenHeight){
          SetScreenSize(NULL, &scrHeight);
          screenHeightINI = scrHeight;
          update_screensize = 1;
        }
        continue;
      }
    }
    if(Match(buffer, "SHOWTIMEBAR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i", &visTimebar,&timebar_overlap);
      continue;
    }
    if(Match(buffer, "SHOWCOLORBARS") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &visColorbarVertical);
      continue;
    }
    if(Match(buffer, "EYEVIEW") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &rotation_type);
      continue;
    }
    if(Match(buffer, "NOPART") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &nopart);
      continue;
    }
    if(Match(buffer, "WINDOWOFFSET") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &titlesafe_offsetBASE);
      continue;
    }
    if(Match(buffer, "LIGHT0") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%d", &light_enabled0);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "LIGHT1") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%d", &light_enabled1);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "LIGHTFACES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%d", &light_faces);
      ONEORZERO(light_faces);
      continue;
    }
    if(Match(buffer, "LIGHTPOS0") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", light_position0, light_position0 + 1, light_position0 + 2, light_position0 + 3);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "LIGHTPOS1") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", light_position1, light_position1 + 1, light_position1 + 2, light_position1 + 3);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "LIGHTMODELLOCALVIEWER") == 1){
      int temp;
      fgets(buffer, 255, stream);
      sscanf(buffer, "%d", &temp);
      lightmodel_localviewer = temp == 0 ? GL_FALSE : GL_TRUE;
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "LIGHTMODELSEPARATESPECULARCOLOR") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%d", &lightmodel_separatespecularcolor);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "AMBIENTLIGHT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", ambientlight, ambientlight + 1, ambientlight + 2);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "DIFFUSELIGHT") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", diffuselight, diffuselight + 1, diffuselight + 2);
      UpdateLIGHTS = 1;
      continue;
    }
    if(Match(buffer, "LABELSTARTUPVIEW") == 1){
      char *front;

      fgets(buffer, 255, stream);
      front = TrimFront(buffer);
      TrimBack(front);
      strcpy(startup_view_label, front);
      update_startup_view = 1;
      continue;
    }
    if(Match(buffer, "USER_ROTATE") == 1){
      if(fgets(buffer, 255, stream) == NULL)break;
      sscanf(buffer, "%i %i %f %f %f", &glui_rotation_index_ini, &show_rotation_center, &xcenCUSTOM, &ycenCUSTOM, &zcenCUSTOM);
      update_rotation_center_ini = 1;
      continue;
    }
    if(Match(buffer, "INPUT_FILE") == 1){
      size_t len;

      if(fgets(buffer, 255, stream) == NULL)break;
      len = strlen(buffer);
      buffer[len - 1] = '\0';
      TrimBack(buffer);
      len = strlen(buffer);

      FREEMEMORY(INI_fds_filein);
      if(NewMemory((void **)&INI_fds_filein, (unsigned int)(len + 1)) == 0)return 2;
      STRCPY(INI_fds_filein, buffer);
      continue;
    }
    if(Match(buffer, "VIEWPOINT5") == 1 || Match(buffer, "VIEWPOINT6") == 1){
      int p_type;
      float *eye, mat[16], *az_elev;
      int is_viewpoint6 = 0;
      float xyzmaxdiff_local = -1.0;
      float xmin_local = 0.0, ymin_local = 0.0, zmin_local = 0.0;
      char name_ini[32];
      float zoom_in;
      int zoomindex_in;
      cameradata *ci;
      char *bufferptr;

      ci = camera_ini;

      if(Match(buffer, "VIEWPOINT6") == 1)is_viewpoint6 = 1;

      eye = ci->eye;
      az_elev = ci->az_elev;

      strcpy(name_ini, "ini");
      InitCamera(ci, name_ini);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %f %f %f %f",
        &ci->rotation_type, &ci->rotation_index, &ci->view_id,
        &xyzmaxdiff_local, &xmin_local, &ymin_local, &zmin_local);

      zoom_in = zoom;
      zoomindex_in = zoomindex;
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f %i", eye, eye + 1, eye + 2, &zoom_in, &zoomindex_in);
      if(xyzmaxdiff_local>0.0){
        eye[0] = xmin_local + eye[0] * xyzmaxdiff_local;
        eye[1] = ymin_local + eye[1] * xyzmaxdiff_local;
        eye[2] = zmin_local + eye[2] * xyzmaxdiff_local;
      }
      zoom = zoom_in;
      zoomindex = zoomindex_in;
      if(zoomindex != -1){
        if(zoomindex<0)zoomindex = 2;
        if(zoomindex>4)zoomindex = 2;
        zoom = zooms[zoomindex];
      }
      else{
        if(zoom<zooms[0]){
          zoom = zooms[0];
          zoomindex = 0;
        }
        if(zoom>zooms[4]){
          zoom = zooms[4];
          zoomindex = 4;
        }
      }
      updatezoommenu = 1;

      p_type = 0;
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %i", &ci->view_angle, &ci->azimuth, &ci->elevation, &p_type);
      if(p_type != 1)p_type = 0;
      ci->projection_type = p_type;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", &ci->xcen, &ci->ycen, &ci->zcen);
      if(xyzmaxdiff_local>0.0){
        ci->xcen = xmin_local + ci->xcen*xyzmaxdiff_local;
        ci->ycen = ymin_local + ci->ycen*xyzmaxdiff_local;
        ci->zcen = zmin_local + ci->zcen*xyzmaxdiff_local;
      }

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f", az_elev, az_elev + 1);

      if(is_viewpoint6 == 1){
        float *q;

        for(i = 0; i<16; i++){
          mat[i] = 0.0;
          if(i % 5 == 0)mat[i] = 1.0;
        }
        q = ci->quaternion;
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %f %f %f %f", &ci->quat_defined, q, q + 1, q + 2, q + 3);
      }

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", mat, mat + 1, mat + 2, mat + 3);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", mat + 4, mat + 5, mat + 6, mat + 7);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", mat + 8, mat + 9, mat + 10, mat + 11);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f", mat + 12, mat + 13, mat + 14, mat + 15);

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i %i %i %i %i %i %i",
        &ci->clip_mode,
        &ci->clip_xmin, &ci->clip_ymin, &ci->clip_zmin,
        &ci->clip_xmax, &ci->clip_ymax, &ci->clip_zmax);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f %f %f %f",
        &ci->xmin, &ci->ymin, &ci->zmin,
        &ci->xmax, &ci->ymax, &ci->zmax);
      if(xyzmaxdiff_local>0.0){
        ci->xmin = xmin_local + ci->xmin*xyzmaxdiff_local;
        ci->xmax = xmin_local + ci->xmax*xyzmaxdiff_local;
        ci->ymin = ymin_local + ci->ymin*xyzmaxdiff_local;
        ci->zmax = ymin_local + ci->ymax*xyzmaxdiff_local;
        ci->ymin = zmin_local + ci->zmin*xyzmaxdiff_local;
        ci->zmax = zmin_local + ci->zmax*xyzmaxdiff_local;
      }

      fgets(buffer, 255, stream);
      TrimBack(buffer);
      bufferptr = TrimFront(buffer);
      strcpy(camera_ini->name, bufferptr);
      InitCameraList();
      InsertCamera(&camera_list_first, camera_ini, bufferptr);

      EnableResetSavedView();
      ci->dirty = 1;
      ci->defined = 1;
      continue;
    }
    if(Match(buffer, "COLORTABLE") == 1){
      int nctableinfo;
      colortabledata *ctableinfo = NULL;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &nctableinfo);
      nctableinfo = MAX(nctableinfo, 0);
      if(nctableinfo>0){
        NewMemory((void **)&ctableinfo, nctableinfo*sizeof(colortabledata));
        for(i = 0; i<nctableinfo; i++){
          colortabledata *rgbi;
          char *labelptr, *percenptr, label[256];
          int  colori[4];


          rgbi = ctableinfo + i;
          fgets(buffer, 255, stream);
          percenptr = strchr(buffer, '%');
          if(percenptr != NULL){
            labelptr = TrimFront(percenptr + 1);
            TrimBack(labelptr);
            strcpy(rgbi->label, labelptr);
            percenptr[0] = 0;
          }
          else{
            sprintf(label, "Color %i", i + 1);
            strcpy(rgbi->label, label);
          }
          colori[3] = 255;
          sscanf(buffer, "%i %i %i %i", colori, colori + 1, colori + 2, colori + 3);
          rgbi->color[0] = CLAMP(colori[0], 0, 255);
          rgbi->color[1] = CLAMP(colori[1], 0, 255);
          rgbi->color[2] = CLAMP(colori[2], 0, 255);
          rgbi->color[3] = CLAMP(colori[3], 0, 255);
        }
        UpdateColorTable(ctableinfo, nctableinfo);
        FREEMEMORY(ctableinfo);
      }
      continue;
    }

    if(Match(buffer, "ISOCOLORS") == 1){
      int nn, n_iso_c = 0;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f", &iso_shininess, &iso_transparency);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%f %f %f", iso_specular, iso_specular + 1, iso_specular + 2);
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &n_iso_c);
      for(nn = 0; nn<MAX_ISO_COLORS; nn++){
        float *iso_color;

        iso_color = iso_colors + 4 * nn;
        if(nn < n_iso_c){
          fgets(buffer, 255, stream);
          sscanf(buffer, "%f %f %f %f", iso_color, iso_color + 1, iso_color + 2, iso_color + 3);
        }
        iso_color[0] = CLAMP(iso_color[0], 0.0, 1.0);
        iso_color[1] = CLAMP(iso_color[1], 0.0, 1.0);
        iso_color[2] = CLAMP(iso_color[2], 0.0, 1.0);
        iso_color[3] = CLAMP(iso_color[3], 0.0, 1.0);
      }
      UpdateIsoColors();
      UpdateIsoColorlevel();
      continue;
    }
    if(Match(buffer, "UNITCLASSES") == 1){
      int nuc;

      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &nuc);
      for(i = 0; i<nuc; i++){
        int unit_index;

        fgets(buffer, 255, stream);
        if(i>nunitclasses - 1)continue;
        sscanf(buffer, "%i", &unit_index);
        unitclasses[i].unit_index = unit_index;
      }
      continue;
    }
    if(Match(buffer, "SMOOTHLINES") == 1){
      fgets(buffer, 255, stream);
      sscanf(buffer, "%i", &antialiasflag);
      continue;
    }
    {
      int nkeyframes;
      float key_time, key_xyz[3], key_az_path, key_view[3], params[3], zzoom, key_elev_path;
      float t_globaltension, key_bank;
      int t_globaltension_flag;
      int viewtype, uselocalspeed;
      float *col;

      if(Match(buffer, "ADJUSTALPHA") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &adjustalphaflag);
        continue;
      }
      if(Match(buffer, "SMOKECULL") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
#ifdef pp_CULL
        if(gpuactive == 1){
          sscanf(buffer, "%i", &cullsmoke);
          if(cullsmoke != 0)cullsmoke = 1;
        }
        else{
          cullsmoke = 0;
        }
#else
        sscanf(buffer, "%i", &smokecullflag);
#endif
        continue;
      }
      if(Match(buffer, "SMOKESKIP") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &smokeskipm1);
        continue;
      }
      if(Match(buffer, "SMOKEALBEDO") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f", &smoke_albedo);
        smoke_albedo = CLAMP(smoke_albedo, 0.0, 1.0);
        continue;
      }
      if(Match(buffer, "SMOKETHICK") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &smoke3d_thick);
        continue;
      }
#ifdef pp_GPU
      if(Match(buffer, "SMOKERTHICK") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f", &smoke3d_rthick);
        smoke3d_rthick = CLAMP(smoke3d_rthick, 1.0, 255.0);
        smoke3d_thick = LogBase2(smoke3d_rthick);
        continue;
      }
#endif
      if(Match(buffer, "FIRECOLOR") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i %i %i", &fire_red, &fire_green, &fire_blue);
        continue;
      }
      if(Match(buffer, "SMOKECOLOR") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i %i %i", &smoke_red, &smoke_green, &smoke_blue);
        continue;
      }
      if(Match(buffer, "FIREDEPTH") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f", &fire_halfdepth,&co2_halfdepth);
        continue;
      }
      if(Match(buffer, "VIEWTOURFROMPATH") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &viewtourfrompath);
        continue;
      }
      if(Match(buffer, "VIEWALLTOURS") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &viewalltours);
        continue;
      }
      if(Match(buffer, "SHOWTOURROUTE") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &edittour);
        continue;
      }
      if(Match(buffer, "TIMEOFFSET") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f", &timeoffset);
        continue;
      }
      if(Match(buffer, "SHOWPATHNODES") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &show_path_knots);
        continue;
      }
      if(Match(buffer, "SHOWIGNITION") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i %i", &vis_threshold, &vis_onlythreshold);
        continue;
      }
      if(Match(buffer, "SHOWTHRESHOLD") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i %i %f", &vis_threshold, &vis_onlythreshold, &temp_threshold);
        continue;
      }
      if(Match(buffer, "TOURCONSTANTVEL") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i", &tour_constant_vel);
        continue;
      }
      if(Match(buffer, "TOUR_AVATAR") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        //        sscanf(buffer,"%i %f %f %f %f",&tourlocus_type,tourcol_avatar,tourcol_avatar+1,tourcol_avatar+2,&tourrad_avatar);
        //        if(tourlocus_type!=0)tourlocus_type=1;
        continue;
      }

      /*
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      ++++++++++++++++++++++ GCOLORBAR ++++++++++++++++++++++++++++
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      */

      if(Match(buffer, "GCOLORBAR") == 1){
        colorbardata *cbi;
        int r1, g1, b1;
        int n;
        int ncolorbarini;

        fgets(buffer, 255, stream);
        ncolorbarini = 0;
        sscanf(buffer, "%i", &ncolorbarini);

        ncolorbarini = MAX(ncolorbarini, 0);
        InitDefaultColorbars(ncolorbarini);

        ncolorbars = ndefaultcolorbars + ncolorbarini;
        for(n = ndefaultcolorbars; n<ncolorbars; n++){
          char *cb_buffptr;

          cbi = colorbarinfo + n;
          fgets(buffer, 255, stream);
          TrimBack(buffer);
          cb_buffptr = TrimFront(buffer);
          strcpy(cbi->label, cb_buffptr);

          fgets(buffer, 255, stream);
          sscanf(buffer, "%i %i", &cbi->nnodes, &cbi->nodehilight);
          if(cbi->nnodes<0)cbi->nnodes = 0;
          if(cbi->nodehilight<0 || cbi->nodehilight >= cbi->nnodes){
            cbi->nodehilight = 0;
          }

          cbi->label_ptr = cbi->label;
          for(i = 0; i<cbi->nnodes; i++){
            int icbar;
            int nn;

            fgets(buffer, 255, stream);
            r1 = -1; g1 = -1; b1 = -1;
            sscanf(buffer, "%i %i %i %i", &icbar, &r1, &g1, &b1);
            cbi->index_node[i] = icbar;
            nn = 3 * i;
            cbi->rgb_node[nn] = r1;
            cbi->rgb_node[nn + 1] = g1;
            cbi->rgb_node[nn + 2] = b1;
          }
          RemapColorbar(cbi);
          UpdateColorbarSplits(cbi);
        }
        continue;
      }

      if(Match(buffer, "TOURCOLORS") == 1){
        col = tourcol_selectedpathline;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        col = tourcol_selectedpathlineknots;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        col = tourcol_selectedknot;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        col = tourcol_pathline;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        col = tourcol_pathknots;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        col = tourcol_text;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        col = tourcol_avatar;
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", col, col + 1, col + 2);

        continue;
      }


      if(localfile!=1)continue;

// ---------------------------------------------------------------------------------------------------------
//   keywords below are 'local', only in the casename.ini file
// ---------------------------------------------------------------------------------------------------------

#define WINDROSE_PER_ROW 10
      if(Match(buffer, "WINDROSESHOWHIDE")==1){
        int i1, i2, *vals, nrows;

        FREEMEMORY(windrose_showhide);
        nwindrose_showhide = 0;
        fgets(buffer, 255, stream);
        sscanf(buffer, " %i", &nwindrose_showhide);
        if(nwindrose_showhide>0){
          nrows = ((nwindrose_showhide-1)/WINDROSE_PER_ROW+1);
          NewMemory((void **)&windrose_showhide, nrows*WINDROSE_PER_ROW*sizeof(int));
          for(vals=windrose_showhide,i=0;i<nrows;i++,vals+=WINDROSE_PER_ROW){
            int j;

            i1 = WINDROSE_PER_ROW*i;
            i2 = MIN(i1+WINDROSE_PER_ROW,nwindrose_showhide);
            fgets(buffer, 255, stream);
            sscanf(buffer, " %i %i %i %i %i %i %i %i %i %i ",
              vals,vals+1,vals+2,vals+3,vals+4,vals+5,vals+6,vals+7,vals+8,vals+9);
            for(j=i1;j<i2;j++){
              windrose_showhide[j] = CLAMP(vals[j-i1],0,1);
            }
          }
          update_windrose_showhide = 1;
        }
      }

      if(Match(buffer, "SMOKE3DCUTOFFS") == 1){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%f %f", &load_3dsmoke_cutoff, &load_hrrpuv_cutoff);
        continue;
      }

      /*
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      ++++++++++++++++++++++ LABEL ++++++++++++++++++++++++++++++++
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      */

      if(Match(buffer, "LABEL") == 1 || Match(buffer, "TICKLABEL") == 1){

        /*
        LABEL
        x y z r g b tstart tstop
        label

        */
        float *xyz, *rgbtemp, *tstart_stop;
        labeldata labeltemp, *labeli;
        int *useforegroundcolor;
        char *bufferptr;
        int *show_always;
        int ticklabel = 0;
        float *xyztick, *xyztickdir;
        int *showtick;

        if(Match(buffer, "TICKLABEL") == 1)ticklabel = 1;

        labeli = &labeltemp;

        labeli->labeltype = TYPE_INI;
        xyz = labeli->xyz;
        rgbtemp = labeli->frgb;
        xyztick = labeli->tick_begin;
        xyztickdir = labeli->tick_direction;
        showtick = &labeli->show_tick;

        useforegroundcolor = &labeli->useforegroundcolor;
        tstart_stop = labeli->tstart_stop;
        show_always = &labeli->show_always;

        fgets(buffer, 255, stream);
        rgbtemp[0] = -1.0;
        rgbtemp[1] = -1.0;
        rgbtemp[2] = -1.0;
        rgbtemp[3] = 1.0;
        tstart_stop[0] = -1.0;
        tstart_stop[1] = -1.0;
        *useforegroundcolor = -1;
        *show_always = 1;

        sscanf(buffer, "%f %f %f %f %f %f %f %f %i %i",
          xyz, xyz + 1, xyz + 2,
          rgbtemp, rgbtemp + 1, rgbtemp + 2,
          tstart_stop, tstart_stop + 1, useforegroundcolor, show_always);

        if(ticklabel == 1){
          fgets(buffer, 255, stream);
          sscanf(buffer, "%f %f %f %f %f %f %i",
            xyztick, xyztick + 1, xyztick + 2,
            xyztickdir, xyztickdir + 1, xyztickdir + 2,
            showtick);
          *showtick = CLAMP(*showtick, 0, 1);
        }
        else{
          xyztick[0] = 0.0;
          xyztick[1] = 0.0;
          xyztick[2] = 0.0;
          xyztickdir[0] = 1.0;
          xyztickdir[1] = 0.0;
          xyztickdir[2] = 0.0;
          *showtick = 0;
        }
        *show_always = CLAMP(*show_always, 0, 1);
        *useforegroundcolor = CLAMP(*useforegroundcolor, -1, 1);
        if(*useforegroundcolor == -1){
          if(rgbtemp[0]<0.0 || rgbtemp[1]<0.0 || rgbtemp[2]<0.0 || rgbtemp[0]>1.0 || rgbtemp[1]>1.0 || rgbtemp[2]>1.0){
            *useforegroundcolor = 1;
          }
          else{
            *useforegroundcolor = 0;
          }
        }
        fgets(buffer, 255, stream);
        TrimBack(buffer);
        bufferptr = TrimFront(buffer);
        strcpy(labeli->name, bufferptr);
        LabelInsert(labeli);
        continue;
      }
      if(Match(buffer, "VIEWTIMES") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %i", &view_tstart, &view_tstop, &view_ntimes);
        if(view_ntimes<2)view_ntimes = 2;
        ReallocTourMemory();
        continue;
      }
      if(Match(buffer, "SHOOTER") == 1){
        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", shooter_xyz, shooter_xyz + 1, shooter_xyz + 2);

        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", shooter_dxyz, shooter_dxyz + 1, shooter_dxyz + 2);

        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", shooter_uvw, shooter_uvw + 1, shooter_uvw + 2);

        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f %f", &shooter_velmag, &shooter_veldir, &shooterpointsize);

        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%i %i %i %i %i", &shooter_fps, &shooter_vel_type, &shooter_nparts, &visShooter, &shooter_cont_update);

        if(fgets(buffer, 255, stream) == NULL)break;
        sscanf(buffer, "%f %f", &shooter_duration, &shooter_v_inf);
        continue;
      }
      if(Match(buffer, "SCRIPTFILE") == 1){
        if(fgets(buffer2, 255, stream) == NULL)break;
        InsertScriptFile(RemoveComment(buffer2));
        updatemenu = 1;
        continue;
      }
      if(Match(buffer, "SHOWDEVICES") == 1){
        sv_object *obj_typei;
        char *dev_label;
        int ndevices_ini;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &ndevices_ini);

        for(i = 0; i<nobject_defs; i++){
          obj_typei = object_defs[i];
          obj_typei->visible = 0;
        }
        for(i = 0; i<ndevices_ini; i++){
          fgets(buffer, 255, stream);
          TrimBack(buffer);
          dev_label = TrimFront(buffer);
          obj_typei = GetSmvObject(dev_label);
          if(obj_typei != NULL){
            obj_typei->visible = 1;
          }
        }
        continue;
      }
      if(Match(buffer, "XYZCLIP") == 1){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &clip_mode);
        clip_mode = CLAMP(clip_mode, 0, 2);
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %f %i %f", &clipinfo.clip_xmin, &clipinfo.xmin, &clipinfo.clip_xmax, &clipinfo.xmax);
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %f %i %f", &clipinfo.clip_ymin, &clipinfo.ymin, &clipinfo.clip_ymax, &clipinfo.ymax);
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %f %i %f", &clipinfo.clip_zmin, &clipinfo.zmin, &clipinfo.clip_zmax, &clipinfo.zmax);
        updateclipvals = 1;
        continue;
      }


      /*
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      ++++++++++++++++++++++ TICKS ++++++++++++++++++++++++++++++++
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      */

      /*
      typedef struct {
      float begin[3],end[3],length;
      float dxyz[3],dlength;
      int dir,nbars;
      } tickdata;
      */

      if(Match(buffer, "TICKS") == 1){
        ntickinfo++;
        ResizeMemory((void **)&tickinfo, (ntickinfo)*sizeof(tickdata));

        {
          tickdata *ticki;
          float *begt, *endt;
          int *nbarst;
          float term;
          float length = 0.0;
          float *dxyz;
          float sum;

          ticki = tickinfo + ntickinfo - 1;
          begt = ticki->begin;
          endt = ticki->end;
          nbarst = &ticki->nbars;
          dxyz = ticki->dxyz;


          /*
          TICKS
          b1 b2 b3 e1 e2 e3 nb
          ticklength tickdir tickcolor (r g b) tickwidth
          */
          if(fgets(buffer, 255, stream) == NULL)break;
          *nbarst = 0;
          sscanf(buffer, "%f %f %f %f %f %f %i", begt, begt + 1, begt + 2, endt, endt + 1, endt + 2, nbarst);
          if(*nbarst<1)*nbarst = 1;
          if(fgets(buffer, 255, stream) == NULL)break;
          {
            float *rgbtemp;

            rgbtemp = ticki->rgb;
            VEC3EQCONS(rgbtemp, -1.0);
            ticki->width = -1.0;
            sscanf(buffer, "%f %i %f %f %f %f", &ticki->dlength, &ticki->dir, rgbtemp, rgbtemp + 1, rgbtemp + 2, &ticki->width);
            if(rgbtemp[0]<0.0 || rgbtemp[0]>1.0 ||
              rgbtemp[1]<0.0 || rgbtemp[1]>1.0 ||
              rgbtemp[2]<0.0 || rgbtemp[2]>1.0){
              ticki->useforegroundcolor = 1;
            }
            else{
              ticki->useforegroundcolor = 0;
            }
            if(ticki->width<0.0)ticki->width = 1.0;
          }
          for(i = 0; i<3; i++){
            term = endt[i] - begt[i];
            length += term*term;
          }
          if(length <= 0.0){
            endt[0] = begt[0] + 1.0;
            length = 1.0;
          }
          ticki->length = sqrt(length);
          VEC3EQCONS(dxyz, 0.0);
          switch(ticki->dir){
          case XLEFT:
          case XRIGHT:
            dxyz[0] = 1.0;
            break;
          case YBACK:
          case YFRONT:
            dxyz[1] = 1.0;
            break;
          case ZBOTTOM:
          case ZTOP:
            dxyz[2] = 1.0;
            break;
          default:
            ASSERT(FFALSE);
            break;
          }
          if(ticki->dir<0){
            for(i = 0; i<3; i++){
              dxyz[i] = -dxyz[i];
            }
          }
          sum = 0.0;
          sum = dxyz[0] * dxyz[0] + dxyz[1] * dxyz[1] + dxyz[2] * dxyz[2];
          if(sum>0.0){
            sum = sqrt(sum);
            dxyz[0] *= (ticki->dlength / sum);
            dxyz[1] *= (ticki->dlength / sum);
            dxyz[2] *= (ticki->dlength / sum);
          }
        }
        continue;
      }
      if(Match(buffer, "PROPINDEX") == 1){
        int nvals;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &nvals);
        for(i = 0; i<nvals; i++){
          propdata *propi;
          int ind, val;

          fgets(buffer, 255, stream);
          sscanf(buffer, "%i %i", &ind, &val);
          if(ind<0 || ind>npropinfo - 1)continue;
          propi = propinfo + ind;
          if(val<0 || val>propi->nsmokeview_ids - 1)continue;
          propi->smokeview_id = propi->smokeview_ids[val];
          propi->smv_object = propi->smv_objects[val];
        }
        for(i = 0; i<npartclassinfo; i++){
          partclassdata *partclassi;

          partclassi = partclassinfo + i;
          UpdatePartClassDepend(partclassi);

        }
        continue;
      }
      if(Match(buffer, "partclassdataVIS") == 1){
        int ntemp;
        int j;

        fgets(buffer, 255, stream);
        sscanf(buffer, "%i", &ntemp);

        for(j = 0; j<ntemp; j++){
          partclassdata *partclassj;

          if(j>npartclassinfo)break;

          partclassj = partclassinfo + j;
          fgets(buffer, 255, stream);
          sscanf(buffer, "%i", &partclassj->vis_type);
        }
        continue;
      }

      if(Match(buffer, "GSLICEPARMS") == 1){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %i %i %i", &vis_gslice_data, &show_gslice_triangles, &show_gslice_triangulation, &show_gslice_normal);
        ONEORZERO(vis_gslice_data);
        ONEORZERO(show_gslice_triangles);
        ONEORZERO(show_gslice_triangulation);
        ONEORZERO(show_gslice_normal);
        fgets(buffer, 255, stream);
        sscanf(buffer, "%f %f %f", gslice_xyz, gslice_xyz + 1, gslice_xyz + 2);
        fgets(buffer, 255, stream);
        sscanf(buffer, "%f %f", gslice_normal_azelev, gslice_normal_azelev + 1);
        update_gslice = 1;
        continue;
      }
      if(Match(buffer, "GEOMETRYTEST") == 1){
        int *v, ii;
        float *b1, *b2, *b3;

        v = tetrabox_vis;
        fgets(buffer, 255, stream);
        sscanf(buffer, " %i %i %f %f", &geomtest_option, &show_tetratest_labels, &tetra_line_thickness, &tetra_point_size);
        fgets(buffer, 255, stream);
        sscanf(buffer, " %i %i %i %i %i ", v, v + 1, v + 2, v + 3, v + 4);
        fgets(buffer, 255, stream);
        sscanf(buffer, " %i %i %i %i %i ", v + 5, v + 6, v + 7, v + 8, v + 9);
        ONEORZERO(show_tetratest_labels);
        for(ii = 0; ii<10; ii++){
          ONEORZERO(v[ii]);
        }
        b1 = box_bounds2;
        b2 = tetra_vertices;
        b3 = box_translate;
        fgets(buffer, 255, stream);
        sscanf(buffer, " %f %f %f %f %f %f", b1, b1 + 1, b1 + 2, b1 + 3, b1 + 4, b1 + 5);
        fgets(buffer, 255, stream);
        sscanf(buffer, " %f %f %f %f %f %f", b2, b2 + 1, b2 + 2, b2 + 3, b2 + 4, b2 + 5);
        fgets(buffer, 255, stream);
        sscanf(buffer, " %f %f %f %f %f %f", b2 + 6, b2 + 7, b2 + 8, b2 + 9, b2 + 10, b2 + 11);
        fgets(buffer, 255, stream);
        sscanf(buffer, " %f %f %f", b3, b3 + 1, b3 + 2);
        continue;
      }
      if(Match(buffer, "GRIDPARMS") == 1){
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %i %i", &visx_all, &visy_all, &visz_all);
        fgets(buffer, 255, stream);
        sscanf(buffer, "%i %i %i", &iplotx_all, &iploty_all, &iplotz_all);
        if(iplotx_all>nplotx_all - 1)iplotx_all = 0;
        if(iploty_all>nploty_all - 1)iploty_all = 0;
        if(iplotz_all>nplotz_all - 1)iplotz_all = 0;
        continue;
      }
      {
        int tours_flag;

        tours_flag = 0;
        if(Match(buffer, "TOURS") == 1)tours_flag = 1;
        if(tours_flag == 1){
          if(ntourinfo > 0){
            for(i = 0; i < ntourinfo; i++){
              tourdata *touri;

              touri = tourinfo + i;
              FreeTour(touri);
            }
            FREEMEMORY(tourinfo);
          }
          ntourinfo = 0;

          fgets(buffer, 255, stream);
          sscanf(buffer, "%i", &ntourinfo);
          ntourinfo++;
          if(ntourinfo > 0){
            if(NewMemory((void **)&tourinfo, ntourinfo*sizeof(tourdata)) == 0)return 2;
            for(i = 0; i < ntourinfo; i++){
              tourdata *touri;

              touri = tourinfo + i;
              touri->path_times = NULL;
              touri->pathnodes = NULL;
            }
          }
          ReallocTourMemory();
          InitCircularTour();
          {
            keyframe *thisframe, *addedframe;
            tourdata *touri;
            int glui_avatar_index_local;

            for(i = 1; i < ntourinfo; i++){
              int j;

              touri = tourinfo + i;
              InitTour(touri);
              fgets(buffer, 255, stream);
              TrimBack(buffer);
              strcpy(touri->label, TrimFront(buffer));

              t_globaltension = touri->global_tension;
              t_globaltension_flag = touri->global_tension_flag;
              fgets(buffer, 255, stream);
              glui_avatar_index_local = 0;
              sscanf(buffer, "%i %i %f %i %i",
                &nkeyframes, &t_globaltension_flag, &t_globaltension, &glui_avatar_index_local, &touri->display2);
              glui_avatar_index_local = CLAMP(glui_avatar_index_local, 0, navatar_types - 1);
              touri->glui_avatar_index = glui_avatar_index_local;
              if(touri->display2 != 1)touri->display2 = 0;
              touri->global_tension_flag = t_globaltension_flag;
              touri->global_tension = t_globaltension;
              touri->nkeyframes = nkeyframes;

              if(NewMemory((void **)&touri->keyframe_times, nkeyframes*sizeof(float)) == 0)return 2;
              if(NewMemory((void **)&touri->pathnodes, view_ntimes*sizeof(pathdata)) == 0)return 2;
              if(NewMemory((void **)&touri->path_times, view_ntimes*sizeof(float)) == 0)return 2;

              thisframe = &touri->first_frame;
              for(j = 0; j < nkeyframes; j++){
                params[0] = 0.0;
                params[1] = 0.0;
                params[2] = 0.0;
                key_view[0] = 0.0;
                key_view[1] = 0.0;
                key_view[2] = 0.0;
                key_bank = 0.0;
                key_az_path = 0.0;
                key_elev_path = 0.0;
                viewtype = 0;
                zzoom = 1.0;
                uselocalspeed = 0;
                fgets(buffer, 255, stream);

                sscanf(buffer, "%f %f %f %f %i",
                  &key_time,
                  key_xyz, key_xyz + 1, key_xyz + 2,
                  &viewtype);

                if(viewtype == 0){
                  sscanf(buffer, "%f %f %f %f %i %f %f %f %f %f %f %f %i",
                    &key_time,
                    key_xyz, key_xyz + 1, key_xyz + 2,
                    &viewtype, &key_az_path, &key_elev_path, &key_bank,
                    params, params + 1, params + 2,
                    &zzoom, &uselocalspeed);
                }
                else{
                  sscanf(buffer, "%f %f %f %f %i %f %f %f %f %f %f %f %i",
                    &key_time,
                    key_xyz, key_xyz + 1, key_xyz + 2,
                    &viewtype, key_view, key_view + 1, key_view + 2,
                    params, params + 1, params + 2,
                    &zzoom, &uselocalspeed);
                }
                if(zzoom<0.25)zzoom = 0.25;
                if(zzoom>4.00)zzoom = 4.0;
                addedframe = AddFrame(thisframe, key_time, key_xyz, key_az_path, key_elev_path,
                  key_bank, params, viewtype, zzoom, key_view);
                thisframe = addedframe;
                touri->keyframe_times[j] = key_time;
              }
            }
          }
          if(tours_flag == 1){
            for(i = 0; i < ntourinfo; i++){
              tourdata *touri;

              touri = tourinfo + i;
              touri->first_frame.next->prev = &touri->first_frame;
              touri->last_frame.prev->next = &touri->last_frame;
            }
            UpdateTourMenuLabels();
            CreateTourPaths();
            UpdateTimes();
            plotstate = GetPlotState(DYNAMIC_PLOTS);
            selectedtour_index = TOURINDEX_MANUAL;
            selected_frame = NULL;
            selected_tour = NULL;
            if(viewalltours == 1)TourMenu(MENU_TOUR_SHOWALL);
          }
          else{
            ntourinfo = 0;
          }
          strcpy(buffer, "1.00000 1.00000 2.0000 0");
          TrimMZeros(buffer);
          continue;
        }
        if(Match(buffer, "TOURINDEX")){
          if(fgets(buffer, 255, stream) == NULL)break;
          sscanf(buffer, "%i", &selectedtour_index_ini);
          if(selectedtour_index_ini < 0)selectedtour_index_ini = -1;
          update_selectedtour_index = 1;
        }
      }
    }

  }
  fclose(stream);
  return 0;

}

/* ------------------ ReadIni ------------------------ */

int ReadIni(char *inifile){
  char smvprogini[1024];
  char *smvprogini_ptr=NULL;

  ntickinfo=ntickinfo_smv;
  strcpy(smvprogini,"");
  if(smokeview_bindir!=NULL)strcat(smvprogini,smokeview_bindir);
  strcat(smvprogini,"smokeview.ini");
  smvprogini_ptr=smokeviewini;
  if(smokeviewini!=NULL){
#ifdef WIN32
    if(STRCMP(smvprogini,smokeviewini)!=0)smvprogini_ptr=smvprogini;
#else
    if(strcmp(smvprogini,smokeviewini)!=0)smvprogini_ptr=smvprogini;
#endif
  }

  // check if config files read in earlier were modifed later

  if(IsFileNewer(smvprogini_ptr,INIfile)==1){
    PRINTF("*** Warning: The configuration file,\n   %s,\n   is newer than %s \n\n",smvprogini_ptr,INIfile);
  }
  if(IsFileNewer(smvprogini_ptr,caseini_filename)==1){
    PRINTF("*** Warning: The configuration file,\n   %s,\n   is newer than %s \n\n",smvprogini_ptr,caseini_filename);
  }
  if(IsFileNewer(INIfile,caseini_filename)==1){
    PRINTF("*** Warning: The configuration file,\n   %s,\n   is newer than %s \n\n",INIfile,caseini_filename);
  }

  // read in config files if they exist

  // smokeview.ini ini in install directory

  if(smvprogini_ptr!=NULL){
    int returnval;

    returnval = ReadIni2(smvprogini_ptr, 0);
    if(returnval==2)return 2;
    if(returnval == 0){
      PRINTF("complete");
      PRINTF("\n");
    }
    UpdateTerrainOptions();
  }

  // smokeview.ini in case directory

  {
    int returnval;

    returnval = ReadIni2(INIfile, 0);
    if(returnval==2)return 2;
    if(returnval == 0){
      PRINTF("complete");
      PRINTF("\n");
    }
  }

  // read in casename.ini

  if(caseini_filename!=NULL){
    int returnval;

    returnval = ReadIni2(caseini_filename, 1);
    if(returnval==2)return 2;
    if(returnval == 0){
      PRINTF("complete");
      PRINTF("\n");
    }
  }

  // read in ini file specified in script

  if(inifile!=NULL){
    int return_code;

    return_code = ReadIni2(inifile,1);
    if(return_code == 0){
      PRINTF("complete");
      PRINTF("\n");
    }

    if(return_code==1||return_code==2){
      if(inifile==NULL){
        fprintf(stderr,"*** Error: Unable to read .ini file\n");
      }
      else{
        fprintf(stderr,"*** Error: Unable to read %s\n",inifile);
      }
    }
    if(return_code==2)return 2;

    UpdateRGBColors(COLORBAR_INDEX_NONE);
  }
  UpdateGlui();
  if(showall_textures==1)TextureShowMenu(MENU_TEXTURE_SHOWALL);
  if(ncolorbars<=ndefaultcolorbars){
    InitDefaultColorbars(0);
  }
  updatezoommenu=1;
  GetSliceParams2();
  return 0;
}

/* ------------------ OutputViewpoints ------------------------ */

void OutputViewpoints(FILE *fileout){
  float *eye, *az_elev, *mat;
  cameradata *ca;

  for(ca = camera_list_first.next; ca->next != NULL; ca = ca->next){
    if(strcmp(ca->name, "internal") == 0)continue;
    if(strcmp(ca->name, "external") == 0)continue;

    if(ca->quat_defined == 1){
      fprintf(fileout, "VIEWPOINT6\n");
    }
    else{
      fprintf(fileout, "VIEWPOINT5\n");
    }
    eye = ca->eye;
    az_elev = ca->az_elev;
    mat = modelview_identity;

    fprintf(fileout, " %i %i %i\n", ca->rotation_type, ca->rotation_index, ca->view_id);
    fprintf(fileout, " %f %f %f %f %i\n", eye[0], eye[1], eye[2], zoom, zoomindex);
    fprintf(fileout, " %f %f %f %i\n", ca->view_angle, ca->azimuth, ca->elevation, ca->projection_type);
    fprintf(fileout, " %f %f %f\n", ca->xcen, ca->ycen, ca->zcen);

    fprintf(fileout, " %f %f\n", az_elev[0], az_elev[1]);
    if(ca->quat_defined == 1){
      fprintf(fileout, " 1 %f %f %f %f\n", ca->quaternion[0], ca->quaternion[1], ca->quaternion[2], ca->quaternion[3]);
    }
    else{
      fprintf(fileout, " %f %f %f %f\n", mat[0], mat[1], mat[2], mat[3]);
      fprintf(fileout, " %f %f %f %f\n", mat[4], mat[5], mat[6], mat[7]);
      fprintf(fileout, " %f %f %f %f\n", mat[8], mat[9], mat[10], mat[11]);
      fprintf(fileout, " %f %f %f %f\n", mat[12], mat[13], mat[14], mat[15]);
    }
    fprintf(fileout, " %i %i %i %i %i %i %i\n",
      ca->clip_mode,
      ca->clip_xmin, ca->clip_ymin, ca->clip_zmin,
      ca->clip_xmax, ca->clip_ymax, ca->clip_zmax);
    fprintf(fileout, " %f %f %f %f %f %f\n",
      ca->xmin, ca->ymin, ca->zmin,
      ca->xmax, ca->ymax, ca->zmax);
    fprintf(fileout, " %s\n", ca->name);
  }
}

  /* ------------------ WriteIniLocal ------------------------ */

void WriteIniLocal(FILE *fileout){
  int i;
  int ndevice_vis = 0;
  sv_object *obj_typei;
  labeldata *thislabel;
  int startup_count;
  scriptfiledata *scriptfile;

  fprintf(fileout, "\n ------------ local ini settings ------------\n\n");

  fprintf(fileout, "AVATAREVAC\n");
  fprintf(fileout, " %i\n", iavatar_evac);
  {
    int *v;
    float *b1, *b2, *b3;

    fprintf(fileout, "GEOMETRYTEST\n");
    v = tetrabox_vis;
    fprintf(fileout, " %i %i %f %f\n", geomtest_option, show_tetratest_labels, tetra_line_thickness, tetra_point_size);
    fprintf(fileout, " %i %i %i %i %i\n", v[0], v[1], v[2], v[3], v[4]);
    fprintf(fileout, " %i %i %i %i %i\n", v[5], v[6], v[7], v[8], v[9]);
    b1 = box_bounds2;
    b2 = tetra_vertices;
    b3 = box_translate;
    fprintf(fileout, " %f %f %f %f %f %f\n", b1[0], b1[1], b1[2], b1[3], b1[4], b1[5]);
    fprintf(fileout, " %f %f %f %f %f %f\n", b2[0], b2[1], b2[2], b2[3], b2[4], b2[5]);
    fprintf(fileout, " %f %f %f %f %f %f\n", b2[6], b2[7], b2[8], b2[9], b2[10], b2[11]);
    fprintf(fileout, " %f %f %f\n", b3[0], b3[1], b3[2]);
  }
  fprintf(fileout, "DEVICEVECTORDIMENSIONS\n");
  fprintf(fileout, " %f %f %f %f\n", vector_baselength, vector_basediameter, vector_headlength, vector_headdiameter);
  fprintf(fileout, "DEVICEBOUNDS\n");
  fprintf(fileout, " %f %f\n", device_valmin, device_valmax);
  fprintf(fileout, "DEVICEORIENTATION\n");
  fprintf(fileout, " %i %f\n", show_device_orientation, orientation_scale);
  fprintf(fileout, "GRIDPARMS\n");
  fprintf(fileout, " %i %i %i\n", visx_all, visy_all, visz_all);
  fprintf(fileout, " %i %i %i\n", iplotx_all, iploty_all, iplotz_all);
  fprintf(fileout, "GSLICEPARMS\n");
  fprintf(fileout, " %i %i %i %i\n", vis_gslice_data, show_gslice_triangles, show_gslice_triangulation, show_gslice_normal);
  fprintf(fileout, " %f %f %f\n", gslice_xyz[0], gslice_xyz[1], gslice_xyz[2]);
  fprintf(fileout, " %f %f\n", gslice_normal_azelev[0], gslice_normal_azelev[1]);
  for(thislabel = label_first_ptr->next; thislabel->next != NULL; thislabel = thislabel->next){
    labeldata *labeli;
    float *xyz, *rgbtemp, *tstart_stop;
    int *useforegroundcolor, *show_always;
    float *xyztick, *xyztickdir;
    int *showtick;

    labeli = thislabel;
    if(labeli->labeltype == TYPE_SMV)continue;
    xyz = labeli->xyz;
    rgbtemp = labeli->frgb;
    tstart_stop = labeli->tstart_stop;
    useforegroundcolor = &labeli->useforegroundcolor;
    show_always = &labeli->show_always;
    xyztick = labeli->tick_begin;
    xyztickdir = labeli->tick_direction;
    showtick = &labeli->show_tick;

    fprintf(fileout, "TICKLABEL\n");
    fprintf(fileout, " %f %f %f %f %f %f %f %f %i %i\n",
      xyz[0], xyz[1], xyz[2],
      rgbtemp[0], rgbtemp[1], rgbtemp[2],
      tstart_stop[0], tstart_stop[1],
      *useforegroundcolor, *show_always);
    fprintf(fileout, " %f %f %f %f %f %f %i\n",
      xyztick[0], xyztick[1], xyztick[2],
      xyztickdir[0], xyztickdir[1], xyztickdir[2],
      *showtick);
    fprintf(fileout, " %s\n", labeli->name);
  }
  fprintf(fileout, "LOADFILESATSTARTUP\n");
  fprintf(fileout, " %i\n", loadfiles_at_startup);
  fprintf(fileout, "MSCALE\n");
  fprintf(fileout, " %f %f %f\n", mscale[0], mscale[1], mscale[2]);
  PutStartupSmoke3d(fileout);
  if(npart5prop > 0){
    fprintf(fileout, "PART5PROPDISP\n");
    for(i = 0; i < npart5prop; i++){
      partpropdata *propi;
      int j;

      propi = part5propinfo + i;
      fprintf(fileout, " ");
      for(j = 0; j < npartclassinfo; j++){
        fprintf(fileout, " %i ", (int)propi->class_vis[j]);
      }
      fprintf(fileout, "\n");
    }
    fprintf(fileout, "PART5COLOR\n");
    for(i = 0; i < npart5prop; i++){
      partpropdata *propi;

      propi = part5propinfo + i;
      if(propi->display == 1){
        fprintf(fileout, " %i\n", i);
        break;
      }
    }

  }

  if(npartclassinfo > 0){
    int j;

    fprintf(fileout, "partclassdataVIS\n");
    fprintf(fileout, " %i\n", npartclassinfo);
    for(j = 0; j<npartclassinfo; j++){
      partclassdata *partclassj;

      partclassj = partclassinfo + j;
      fprintf(fileout, " %i\n", partclassj->vis_type);
    }
  }

  if(npropinfo>0){
    fprintf(fileout, "PROPINDEX\n");
    fprintf(fileout, " %i\n", npropinfo);
    for(i = 0; i < npropinfo; i++){
      propdata *propi;
      int offset;
      int jj;

      propi = propinfo + i;
      offset = -1;
      for(jj = 0; jj < propi->nsmokeview_ids; jj++){
        if(strcmp(propi->smokeview_id, propi->smokeview_ids[jj]) == 0){
          offset = jj;
          break;
        }
      }
      fprintf(fileout, " %i %i\n", i, offset);
    }
  }

  for(scriptfile = first_scriptfile.next; scriptfile->next != NULL; scriptfile = scriptfile->next){
    char *file;

    file = scriptfile->file;
    if(file != NULL){
      fprintf(fileout, "SCRIPTFILE\n");
      fprintf(fileout, " %s\n", file);
    }
  }
  fprintf(fileout, "SHOOTER\n");
  fprintf(fileout, " %f %f %f\n", shooter_xyz[0], shooter_xyz[1], shooter_xyz[2]);
  fprintf(fileout, " %f %f %f\n", shooter_dxyz[0], shooter_dxyz[1], shooter_dxyz[2]);
  fprintf(fileout, " %f %f %f\n", shooter_uvw[0], shooter_uvw[1], shooter_uvw[2]);
  fprintf(fileout, " %f %f %f\n", shooter_velmag, shooter_veldir, shooterpointsize);
  fprintf(fileout, " %i %i %i %i %i\n", shooter_fps, shooter_vel_type, shooter_nparts, visShooter, shooter_cont_update);
  fprintf(fileout, " %f %f\n", shooter_duration, shooter_v_inf);

  for(i = 0; i < nobject_defs; i++){
    obj_typei = object_defs[i];
    if(obj_typei->used == 1 && obj_typei->visible == 1){
      ndevice_vis++;
    }
  }
  fprintf(fileout, "SHOWDEVICES\n");
  fprintf(fileout, " %i\n", ndevice_vis);
  for(i = 0; i < nobject_defs; i++){
    obj_typei = object_defs[i];
    if(obj_typei->used == 1 && obj_typei->visible == 1){
      fprintf(fileout, " %s\n", obj_typei->label);
    }
  }
  fprintf(fileout, "SHOWDEVICEVALS\n");
  fprintf(fileout, " %i %i %i %i %i %i %i %i %i\n", showdevice_val, showvdevice_val, devicetypes_index, colordevice_val, vectortype, viswindrose, showdevice_type,showdevice_unit,showdevice_id);
  fprintf(fileout, "SHOWMISSINGOBJECTS\n");
  fprintf(fileout, " %i\n", show_missing_objects);
  fprintf(fileout, "SMOKE3DCUTOFFS\n");
  fprintf(fileout, " %f %f\n", load_3dsmoke_cutoff, load_hrrpuv_cutoff);
  for(i = ntickinfo_smv; i < ntickinfo; i++){
    float *begt;
    float *endt;
    float *rgbtemp;
    tickdata *ticki;

    ticki = tickinfo + i;
    begt = ticki->begin;
    endt = ticki->end;
    rgbtemp = ticki->rgb;

    fprintf(fileout, "TICKS\n");
    fprintf(fileout, " %f %f %f %f %f %f %i\n", begt[0], begt[1], begt[2], endt[0], endt[1], endt[2], ticki->nbars);
    fprintf(fileout, " %f %i %f %f %f %f\n", ticki->dlength, ticki->dir, rgbtemp[0], rgbtemp[1], rgbtemp[2], ticki->width);
  }

  fprintf(fileout, "TOURINDEX\n");
  fprintf(fileout, " %i\n", selectedtour_index);
  startup_count = 0;
  for(i = 0; i < ntourinfo; i++){
    tourdata *touri;

    touri = tourinfo + i;
    if(touri->startup == 1)startup_count++;
  }
  if(startup_count < ntourinfo){
    fprintf(fileout, "TOURS\n");
    fprintf(fileout, " %i\n", ntourinfo - startup_count);
    for(i = 0; i < ntourinfo; i++){
      tourdata *touri;
      keyframe *framei;
      int j;

      touri = tourinfo + i;
      if(touri->startup == 1)continue;

      TrimBack(touri->label);
      fprintf(fileout, " %s\n", touri->label);
      fprintf(fileout, " %i %i %f %i %i\n",
        touri->nkeyframes, touri->global_tension_flag, touri->global_tension, touri->glui_avatar_index, touri->display);

      framei = &touri->first_frame;
      for(j = 0; j<touri->nkeyframes; j++){
        char buffer[1024];
        int uselocalspeed = 0;

        framei = framei->next;
        sprintf(buffer, "%f %f %f %f ",
          framei->noncon_time,
          DENORMALIZE_X(framei->nodeval.eye[0]),
          DENORMALIZE_Y(framei->nodeval.eye[1]),
          DENORMALIZE_Z(framei->nodeval.eye[2]));
        TrimMZeros(buffer);
        fprintf(fileout, " %s %i ", buffer, framei->viewtype);
        if(framei->viewtype == REL_VIEW){
          sprintf(buffer, "%f %f %f %f %f %f %f ",
            framei->az_path, framei->nodeval.elev_path, framei->bank,
            framei->tension, framei->bias, framei->continuity,
            framei->nodeval.zoom);
        }
        else{
          sprintf(buffer, "%f %f %f %f %f %f %f ",
            DENORMALIZE_X(framei->nodeval.xyz_view_abs[0]),
            DENORMALIZE_Y(framei->nodeval.xyz_view_abs[1]),
            DENORMALIZE_Z(framei->nodeval.xyz_view_abs[2]),
            framei->tension, framei->bias, framei->continuity,
            framei->nodeval.zoom);
        }
        TrimMZeros(buffer);
        fprintf(fileout, " %s %i\n", buffer, uselocalspeed);
      }
    }
  }
  fprintf(fileout, "USERTICKS\n");
  fprintf(fileout, " %i %i %i %i %i %i %f\n", visUSERticks, auto_user_tick_placement, user_tick_sub,
    user_tick_show_x, user_tick_show_y, user_tick_show_z, user_tick_direction);
  fprintf(fileout, " %f %f %f\n", user_tick_origin[0], user_tick_origin[1], user_tick_origin[2]);
  fprintf(fileout, " %f %f %f\n", user_tick_min[0], user_tick_min[1], user_tick_min[2]);
  fprintf(fileout, " %f %f %f\n", user_tick_max[0], user_tick_max[1], user_tick_max[2]);
  fprintf(fileout, " %f %f %f\n", user_tick_step[0], user_tick_step[1], user_tick_step[2]);
  fprintf(fileout, " %i %i %i\n", user_tick_show_x, user_tick_show_y, user_tick_show_z);
  fprintf(fileout, "XYZCLIP\n");
  fprintf(fileout, " %i\n", clip_mode);
  fprintf(fileout, " %i %f %i %f\n", clipinfo.clip_xmin, clipinfo.xmin, clipinfo.clip_xmax, clipinfo.xmax);
  fprintf(fileout, " %i %f %i %f\n", clipinfo.clip_ymin, clipinfo.ymin, clipinfo.clip_ymax, clipinfo.ymax);
  fprintf(fileout, " %i %f %i %f\n", clipinfo.clip_zmin, clipinfo.zmin, clipinfo.clip_zmax, clipinfo.zmax);

  fprintf(fileout, "\n *** TIME/DATA BOUNDS ***\n");
  fprintf(fileout, "  (0/1 min max skip (1=set, 0=unset)\n\n");

  for(i = 0; i < npatch2; i++){
    int ii;
    patchdata *patchi;

    ii = patchlabellist_index[i];
    patchi = patchinfo + ii;
    fprintf(fileout, "C_BOUNDARY\n");
    fprintf(fileout, " %i %f %i %f %s\n",
      patchi->setchopmin, patchi->chopmin,
      patchi->setchopmax, patchi->chopmax,
      patchi->label.shortlabel
      );
  }
  if(niso_bounds > 0){
    for(i = 0; i < niso_bounds; i++){
      fprintf(fileout, "C_ISO\n");
      fprintf(fileout, " %i %f %i %f %s\n",
        isobounds[i].setchopmin, isobounds[i].chopmin,
        isobounds[i].setchopmax, isobounds[i].chopmax,
        isobounds[i].label->shortlabel
        );
    }
  }
  fprintf(fileout, "C_PARTICLES\n");
  fprintf(fileout, " %i %f %i %f\n", setpartchopmin, partchopmin, setpartchopmax, partchopmax);
  for(i = 0; i < npart5prop; i++){
    partpropdata *propi;

    propi = part5propinfo + i;
    fprintf(fileout, "C_PARTICLES\n");
    fprintf(fileout, " %i %f %i %f %s\n", propi->setchopmin, propi->chopmin, propi->setchopmax, propi->chopmax, propi->label->shortlabel);
  }
  {
    int n3d;

    n3d = 5;
    if(n3d<numplot3dvars)n3d = numplot3dvars;
    if(n3d>mxplot3dvars)n3d = mxplot3dvars;
    fprintf(fileout, "C_PLOT3D\n");
    n3d = 5;
    if(n3d<numplot3dvars)n3d = numplot3dvars;
    if(n3d>mxplot3dvars)n3d = mxplot3dvars;
    fprintf(fileout, " %i\n", n3d);
    for(i = 0; i<n3d; i++){
      fprintf(fileout, " %i %i %f %i %f\n", i + 1, setp3chopmin[i], p3chopmin[i], setp3chopmax[i], p3chopmax[i]);
    }
  }
  if(nslice_type > 0){
    for(i = 0; i < nslice_type; i++){
      fprintf(fileout, "C_SLICE\n");
      fprintf(fileout, " %i %f %i %f %s\n",
        slicebounds[i].setchopmin, slicebounds[i].chopmin,
        slicebounds[i].setchopmax, slicebounds[i].chopmax,
        slicebounds[i].label->shortlabel
        );
    }
  }
  fprintf(fileout, "CACHE_BOUNDARYDATA\n");
  fprintf(fileout, " %i \n", cache_boundarydata);
  fprintf(fileout, "CACHE_QDATA\n");
  fprintf(fileout, " %i\n", cache_qdata);
  fprintf(fileout, "PATCHDATAOUT\n");
  fprintf(fileout, " %i %f %f %f %f %f %f %f %f\n", output_patchdata,
    patchout_tmin, patchout_tmax,
    patchout_xmin, patchout_xmax,
    patchout_ymin, patchout_ymax,
    patchout_zmin, patchout_zmax
    );
  fprintf(fileout, "PERCENTILELEVEL\n");
  fprintf(fileout, " %f\n", percentile_level);
  fprintf(fileout, "TIMEOFFSET\n");
  fprintf(fileout, " %f\n", timeoffset);
  fprintf(fileout, "TLOAD\n");
  fprintf(fileout, " %i %f %i %f %i %i\n", use_tload_begin, tload_begin, use_tload_end, tload_end, use_tload_skip, tload_skip);
  for(i = 0; i < npatchinfo; i++){
    patchdata *patchi;

    patchi = patchinfo + i;
    if(patchi->firstshort == 1){
      fprintf(fileout, "V_BOUNDARY\n");
      fprintf(fileout, " %i %f %i %f %s\n",
        patchi->setvalmin, patchi->valmin,
        patchi->setvalmax, patchi->valmax,
        patchi->label.shortlabel
        );
    }
  }
  if(niso_bounds > 0){
    for(i = 0; i < niso_bounds; i++){
      fprintf(fileout, "V_ISO\n");
      fprintf(fileout, " %i %f %i %f %s\n",
        isobounds[i].setvalmin, isobounds[i].valmin,
        isobounds[i].setvalmax, isobounds[i].valmax,
        isobounds[i].label->shortlabel
        );
    }
  }
  fprintf(fileout, "V_PARTICLES\n");
  fprintf(fileout, " %i %f %i %f\n", setpartmin, partmin, setpartmax, partmax);
  if(npart5prop > 0){
    for(i = 0; i < npart5prop; i++){
      partpropdata *propi;

      propi = part5propinfo + i;
      fprintf(fileout, "V5_PARTICLES\n");
      fprintf(fileout, " %i %f %i %f %s\n",
        propi->setvalmin, propi->valmin, propi->setvalmax, propi->valmax, propi->label->shortlabel);
    }
  }
  {
    int n3d;

    n3d = 5;
    if(n3d<numplot3dvars)n3d = numplot3dvars;
    if(n3d>mxplot3dvars)n3d = mxplot3dvars;
    fprintf(fileout, "V_PLOT3D\n");
    fprintf(fileout, " %i\n", n3d);
    for(i = 0; i < n3d; i++){
      fprintf(fileout, " %i %i %f %i %f\n", i + 1, setp3min[i], p3min[i], setp3max[i], p3max[i]);
    }
  }
  if(nslice_type > 0){
    for(i = 0; i < nslice_type; i++){
      fprintf(fileout, "V_SLICE\n");
      fprintf(fileout, " %i %f %i %f %s : %f %f %i\n",
        slicebounds[i].setvalmin, slicebounds[i].valmin,
        slicebounds[i].setvalmax, slicebounds[i].valmax,
        slicebounds[i].label->shortlabel
        , slicebounds[i].line_contour_min, slicebounds[i].line_contour_max, slicebounds[i].line_contour_num
        );
    }
  }
  fprintf(fileout, "V_TARGET\n");
  fprintf(fileout, " %i %f %i %f\n", settargetmin, targetmin, settargetmax, targetmax);
  if(nzoneinfo > 0){
    fprintf(fileout, "V_ZONE\n");
    fprintf(fileout, " %i %f %i %f\n", setzonemin, zoneusermin, setzonemax, zoneusermax);
    fprintf(fileout, "ZONEVIEW\n");
    fprintf(fileout, " %f\n", zone_hvac_diam);
  }

}

  /* ------------------ WriteIni ------------------------ */

void WriteIni(int flag,char *filename){
  FILE *fileout=NULL;
  int i;

  {

    char *outfilename=NULL;

    switch(flag){
    case GLOBAL_INI:
      fileout=fopen(INIfile,"w");
      outfilename=INIfile;
      break;
    case STDOUT_INI:
      fileout=stdout;
      break;
    case SCRIPT_INI:
      fileout=fopen(filename,"w");
      outfilename=filename;
      break;
    case LOCAL_INI:
      fileout=fopen(caseini_filename,"w");
      outfilename=caseini_filename;
      break;
    default:
      ASSERT(FFALSE);
      break;
    }
    if(flag==SCRIPT_INI)flag=LOCAL_INI;
    if(fileout==NULL){
      if(outfilename!=NULL){
        fprintf(stderr,"*** Error: unable to open %s for writing\n",outfilename);
        return;
      }
      else{
        fprintf(stderr,"*** Error: unable to open ini file for output\n");
      }
    }
  }


  fprintf(fileout,"# NIST Smokeview configuration file, Release %s\n\n",__DATE__);
  fprintf(fileout, "\n ------------ global ini settings ------------\n\n");

  fprintf(fileout,"   *** COLOR/LIGHTING ***\n\n");

  fprintf(fileout, "AMBIENTLIGHT\n");
  fprintf(fileout, " %f %f %f\n", ambientlight[0], ambientlight[1], ambientlight[2]);
  fprintf(fileout, "BACKGROUNDCOLOR\n");
  fprintf(fileout, " %f %f %f\n", backgroundbasecolor[0], backgroundbasecolor[1], backgroundbasecolor[2]);
  fprintf(fileout, "BLOCKCOLOR\n");
  fprintf(fileout, " %f %f %f\n", block_ambient2[0], block_ambient2[1], block_ambient2[2]);
  fprintf(fileout, "BLOCKSHININESS\n");
  fprintf(fileout, " %f\n", block_shininess);
  fprintf(fileout, "BLOCKSPECULAR\n");
  fprintf(fileout, " %f %f %f\n", block_specular2[0], block_specular2[1], block_specular2[2]);
  fprintf(fileout, "BOUNDCOLOR\n");
  fprintf(fileout, " %f %f %f\n", boundcolor[0], boundcolor[1], boundcolor[2]);
  fprintf(fileout, "COLORBAR\n");
  fprintf(fileout," %i %i %i %i\n",nrgb,usetexturebar,colorbar_select_index,colorband);
  for(i=0;i<nrgb;i++){
    fprintf(fileout," %f %f %f\n",rgb[i][0],rgb[i][1],rgb[i][2]);
  }
  fprintf(fileout,"COLOR2BAR\n");
  fprintf(fileout," %i\n",8);
  fprintf(fileout," %f %f %f :white  \n",rgb2[0][0],rgb2[0][1],rgb2[0][2]);
  fprintf(fileout," %f %f %f :yellow \n",rgb2[1][0],rgb2[1][1],rgb2[1][2]);
  fprintf(fileout," %f %f %f :blue   \n",rgb2[2][0],rgb2[2][1],rgb2[2][2]);
  fprintf(fileout," %f %f %f :red    \n",rgb2[3][0],rgb2[3][1],rgb2[3][2]);
  fprintf(fileout," %f %f %f :green  \n",rgb2[4][0],rgb2[4][1],rgb2[4][2]);
  fprintf(fileout," %f %f %f :magenta\n",rgb2[5][0],rgb2[5][1],rgb2[5][2]);
  fprintf(fileout," %f %f %f :cyan   \n",rgb2[6][0],rgb2[6][1],rgb2[6][2]);
  fprintf(fileout," %f %f %f :black  \n",rgb2[7][0],rgb2[7][1],rgb2[7][2]);
  fprintf(fileout, "COLORBAR_FLIP\n");
  fprintf(fileout, " %i %i\n", colorbar_flip,colorbar_autoflip);
  fprintf(fileout, "COLORBAR_SPLIT\n");
  fprintf(fileout, " %i %i %i %i %i %i\n", colorsplit[0], colorsplit[1], colorsplit[2], colorsplit[3], colorsplit[4], colorsplit[5]);
  fprintf(fileout, " %i %i %i %i %i %i\n", colorsplit[6], colorsplit[7], colorsplit[8], colorsplit[9], colorsplit[10], colorsplit[11]);
  fprintf(fileout, " %f %f %f\n", splitvals[0], splitvals[1], splitvals[2]);
  fprintf(fileout,"CO2COLOR\n");
  fprintf(fileout," %i %i %i", global_co2color[0],global_co2color[1],global_co2color[2]);
  fprintf(fileout, "DIFFUSELIGHT\n");
  fprintf(fileout, " %f %f %f\n", diffuselight[0], diffuselight[1], diffuselight[2]);
  fprintf(fileout, "DIRECTIONCOLOR\n");
  fprintf(fileout, " %f %f %f\n", direction_color[0], direction_color[1], direction_color[2]);
  fprintf(fileout, "FLIP\n");
  fprintf(fileout, " %i\n", background_flip);
  fprintf(fileout, "FOREGROUNDCOLOR\n");
  fprintf(fileout, " %f %f %f\n", foregroundbasecolor[0], foregroundbasecolor[1], foregroundbasecolor[2]);
  fprintf(fileout, "HEATOFFCOLOR\n");
  fprintf(fileout, " %f %f %f\n", heatoffcolor[0], heatoffcolor[1], heatoffcolor[2]);
  fprintf(fileout, "HEATONCOLOR\n");
  fprintf(fileout, " %f %f %f\n", heatoncolor[0], heatoncolor[1], heatoncolor[2]);
  fprintf(fileout, "ISOCOLORS\n");
  fprintf(fileout," %f %f : shininess, default opaqueness\n",iso_shininess, iso_transparency);
  fprintf(fileout," %f %f %f : specular\n",iso_specular[0],iso_specular[1],iso_specular[2]);
  fprintf(fileout," %i : number of levels\n",MAX_ISO_COLORS);
  for(i=0;i<MAX_ISO_COLORS;i++){
    fprintf(fileout, " %f %f %f %f", iso_colors[4*i], iso_colors[4*i+1], iso_colors[4*i+2], iso_colors[4*i+3]);
    if(i==0)fprintf(fileout, " : red, green, blue, alpha (opaqueness)");
    fprintf(fileout, "\n");
  }
  if(ncolortableinfo>0){
    char percen[2];

    strcpy(percen,"%");
    fprintf(fileout, "COLORTABLE\n");
    fprintf(fileout, " %i \n", ncolortableinfo);

    for(i = 0; i<ncolortableinfo; i++){
      colortabledata *rgbi;

      rgbi = colortableinfo+i;
      fprintf(fileout, " %i %i %i %i %s %s\n",
        rgbi->color[0], rgbi->color[1], rgbi->color[2], rgbi->color[3], percen, rgbi->label);
    }
  }
  fprintf(fileout, "LIGHT0\n");
  fprintf(fileout, " %i\n", light_enabled0);
  fprintf(fileout, "LIGHT1\n");
  fprintf(fileout, " %i\n", light_enabled1);
  fprintf(fileout, "LIGHTFACES\n");
  fprintf(fileout, " %i\n", light_faces);
  fprintf(fileout, "LIGHTMODELLOCALVIEWER\n");
  fprintf(fileout, " %i\n", lightmodel_localviewer);
  fprintf(fileout, "LIGHTMODELSEPARATESPECULARCOLOR\n");
  fprintf(fileout, " %i\n", lightmodel_separatespecularcolor);
  fprintf(fileout, "LIGHTPOS0\n");
  fprintf(fileout, " %f %f %f %f\n", light_position0[0], light_position0[1], light_position0[2], light_position0[3]);
  fprintf(fileout, "LIGHTPOS1\n");
  fprintf(fileout, " %f %f %f %f\n", light_position1[0], light_position1[1], light_position1[2], light_position1[3]);
  fprintf(fileout, "SENSORCOLOR\n");
  fprintf(fileout, " %f %f %f\n", sensorcolor[0], sensorcolor[1], sensorcolor[2]);
  fprintf(fileout, "SENSORNORMCOLOR\n");
  fprintf(fileout, " %f %f %f\n", sensornormcolor[0], sensornormcolor[1], sensornormcolor[2]);
  fprintf(fileout, "SETBW\n");
  fprintf(fileout, " %i %i\n", setbw,setbwdata);
  fprintf(fileout, "SPRINKOFFCOLOR\n");
  fprintf(fileout, " %f %f %f\n", sprinkoffcolor[0], sprinkoffcolor[1], sprinkoffcolor[2]);
  fprintf(fileout, "SPRINKONCOLOR\n");
  fprintf(fileout, " %f %f %f\n", sprinkoncolor[0], sprinkoncolor[1], sprinkoncolor[2]);
  fprintf(fileout, "STATICPARTCOLOR\n");
  fprintf(fileout, " %f %f %f\n", static_color[0], static_color[1], static_color[2]);
  fprintf(fileout, "TIMEBARCOLOR\n");
  fprintf(fileout, " %f %f %f\n", timebarcolor[0], timebarcolor[1], timebarcolor[2]);
  fprintf(fileout, "VENTCOLOR\n");
  fprintf(fileout," %f %f %f\n",ventcolor[0],ventcolor[1],ventcolor[2]);


/*  extern GLfloat iso_ambient[4], iso_specular[4], iso_shininess;*/



  fprintf(fileout, "\n   *** SIZES/OFFSETS ***\n\n");

  fprintf(fileout, "GRIDLINEWIDTH\n");
  fprintf(fileout, " %f\n", gridlinewidth);
  fprintf(fileout, "ISOLINEWIDTH\n");
  fprintf(fileout, " %f\n", isolinewidth);
  fprintf(fileout, "ISOPOINTSIZE\n");
  fprintf(fileout, " %f\n", isopointsize);
  fprintf(fileout, "LINEWIDTH\n");
  fprintf(fileout, " %f\n", linewidth);
  fprintf(fileout, "PARTPOINTSIZE\n");
  fprintf(fileout, " %f\n", partpointsize);
  fprintf(fileout, "PLOT3DLINEWIDTH\n");
  fprintf(fileout, " %f\n", plot3dlinewidth);
  fprintf(fileout, "PLOT3DPOINTSIZE\n");
  fprintf(fileout, " %f\n", plot3dpointsize);
  fprintf(fileout, "SENSORABSSIZE\n");
  fprintf(fileout, " %f\n", sensorabssize);
  fprintf(fileout, "SENSORRELSIZE\n");
  fprintf(fileout, " %f\n", sensorrelsize);
  fprintf(fileout, "SLICEOFFSET\n");
  fprintf(fileout, " %f\n", sliceoffset_factor);
  fprintf(fileout, "SMOOTHLINES\n");
  fprintf(fileout, " %i\n", antialiasflag);
  fprintf(fileout, "SPHERESEGS\n");
  fprintf(fileout, " %i\n", device_sphere_segments);
  fprintf(fileout, "SPRINKLERABSSIZE\n");
  fprintf(fileout, " %f\n", sprinklerabssize);
  fprintf(fileout, "STREAKLINEWIDTH\n");
  fprintf(fileout, " %f\n", streaklinewidth);
  fprintf(fileout, "TICKLINEWIDTH\n");
  fprintf(fileout, " %f\n", ticklinewidth);
  fprintf(fileout, "USENEWDRAWFACE\n");
  fprintf(fileout, " %i\n", use_new_drawface);
  fprintf(fileout, "VECCONTOURS\n");
  fprintf(fileout, " %i %i\n", show_node_slices_and_vectors,show_cell_slices_and_vectors);
  fprintf(fileout, "VECLENGTH\n");
  fprintf(fileout, " %i %f 1.0\n", 4, vecfactor);
  fprintf(fileout, "VECTORCOLOR\n");
  fprintf(fileout, " %i\n", color_vector_black);
  fprintf(fileout, "VECTORLINEWIDTH\n");
  fprintf(fileout, " %f %f\n", vectorlinewidth, slice_line_contour_width);
  fprintf(fileout, "VECTORPOINTSIZE\n");
  fprintf(fileout," %f\n",vectorpointsize);
  fprintf(fileout, "VENTLINEWIDTH\n");
  fprintf(fileout, " %f\n", ventlinewidth);
  fprintf(fileout, "VENTOFFSET\n");
  fprintf(fileout, " %f\n", ventoffset_factor);
  fprintf(fileout, "WINDOWOFFSET\n");
  fprintf(fileout, " %i\n", titlesafe_offsetBASE);
  if(use_graphics == 1 &&
     (screenWidth == glutGet(GLUT_SCREEN_WIDTH)||screenHeight == glutGet(GLUT_SCREEN_HEIGHT))
    ){
    fprintf(fileout,"WINDOWWIDTH\n");
    fprintf(fileout," %i\n",-1);
    fprintf(fileout,"WINDOWHEIGHT\n");
    fprintf(fileout," %i\n",-1);
  }
  else{
    fprintf(fileout,"WINDOWWIDTH\n");
    fprintf(fileout," %i\n",screenWidth);
    fprintf(fileout,"WINDOWHEIGHT\n");
    fprintf(fileout," %i\n",screenHeight);
  }

  fprintf(fileout, "\n *** DATA LOADING ***\n\n");

  fprintf(fileout, "BOUNDZIPSTEP\n");
  fprintf(fileout, " %i\n", boundzipstep);
  fprintf(fileout, "FED\n");
  fprintf(fileout," %i\n",regenerate_fed);
  fprintf(fileout, "FEDCOLORBAR\n");
  fprintf(fileout, " %s\n", default_fed_colorbar);
  fprintf(fileout, "ISOZIPSTEP\n");
  fprintf(fileout, " %i\n", isozipstep);
  fprintf(fileout, "LOADINC\n");
  fprintf(fileout, " %i\n", load_incremental);
  fprintf(fileout, "NOPART\n");
  fprintf(fileout, " %i\n", nopart);
  fprintf(fileout, "SHOWFEDAREA\n");
  fprintf(fileout, " %i\n", show_fed_area);
  fprintf(fileout, "SLICEAVERAGE\n");
  fprintf(fileout, " %i %f %i\n", slice_average_flag, slice_average_interval, vis_slice_average);
  fprintf(fileout, "SLICEDATAOUT\n");
  fprintf(fileout, " %i \n", output_slicedata);
  fprintf(fileout, "SLICEZIPSTEP\n");
  fprintf(fileout, " %i\n", slicezipstep);
  fprintf(fileout, "SMOKE3DZIPSTEP\n");
  fprintf(fileout, " %i\n", smoke3dzipstep);
  fprintf(fileout, "USER_ROTATE\n");
  fprintf(fileout, " %i %i %f %f %f\n", glui_rotation_index, show_rotation_center, xcenCUSTOM, ycenCUSTOM, zcenCUSTOM);

  fprintf(fileout,"\n *** VIEW PARAMETERS ***\n\n");

  GetGeomDialogState();
  fprintf(fileout, "APERTURE\n");
  fprintf(fileout, " %i\n", apertureindex);
  fprintf(fileout, "AXISSMOOTH\n");
  fprintf(fileout, " %i\n", axislabels_smooth);
  fprintf(fileout, "BLOCKLOCATION\n");
  fprintf(fileout, " %i\n", blocklocation);
  fprintf(fileout, "BEAM\n");
  fprintf(fileout, " %i %f %i %i %i %i\n", showbeam_as_line,beam_line_width,use_beamcolor,beam_color[0], beam_color[1], beam_color[2]);
  fprintf(fileout, "BLENDMODE\n");
  fprintf(fileout, " %i %i %i\n", slices3d_max_blending, hrrpuv_max_blending,showall_3dslices);
  fprintf(fileout, "BOUNDARYTWOSIDE\n");
  fprintf(fileout, " %i\n", showpatch_both);
  fprintf(fileout, "CLIP\n");
  fprintf(fileout, " %f %f\n", nearclip, farclip);
  fprintf(fileout, "CONTOURTYPE\n");
  fprintf(fileout, " %i\n", contour_type);
  fprintf(fileout, "CULLFACES\n");
  fprintf(fileout, " %i\n", cullfaces);
  fprintf(fileout, "ENABLETEXTURELIGHTING\n");
  fprintf(fileout, " %i\n", enable_texture_lighting);
  fprintf(fileout, "EYEVIEW\n");
  fprintf(fileout, " %i\n", rotation_type);
  fprintf(fileout, "EYEX\n");
  fprintf(fileout, " %f\n", eyexfactor);
  fprintf(fileout, "EYEY\n");
  fprintf(fileout, " %f\n", eyeyfactor);
  fprintf(fileout, "EYEZ\n");
  fprintf(fileout, " %f\n", eyezfactor);
  fprintf(fileout, "FONTSIZE\n");
  fprintf(fileout, " %i\n", fontindex);
  fprintf(fileout, "FIREPARAMS\n");
  fprintf(fileout, " %f %f %f %f %f %f %f %f\n",
      slicehrrpuv_lower, slicehrrpuv_middle, slicehrrpuv_upper,
      slicehrrpuv_cut1, slicehrrpuv_cut2, slicehrrpuv_offset,
      voltemp_factor,voltemp_offset);
  fprintf(fileout, "FRAMERATEVALUE\n");
  fprintf(fileout, " %i\n", frameratevalue);
  fprintf(fileout, "FREEZEVOLSMOKE\n");
  fprintf(fileout, " %i %i\n", freeze_volsmoke, autofreeze_volsmoke);
  fprintf(fileout, "GEOMCELLPROPS\n");
  fprintf(fileout, " %i\n",
    immersed_celltype);
  fprintf(fileout, " %i %i %i\n",
    immersed_edgetypes[0], immersed_edgetypes[1], immersed_edgetypes[2]);
  fprintf(fileout, " %i %i %i\n",
    show_immersed_shaded[0], show_immersed_shaded[1], show_immersed_shaded[2]);
  fprintf(fileout, " %i %i %i\n",
    show_immersed_outlines[0], show_immersed_outlines[1], show_immersed_outlines[2]);
  fprintf(fileout, " %i %i %i\n",
    show_immersed_points[0], show_immersed_points[1], show_immersed_points[2]);
  fprintf(fileout, "GEOMDIAGS\n");
  fprintf(fileout, " %i %i %i %i %i %i %i\n", structured_isopen, unstructured_isopen, show_geometry_diagnostics,
    highlight_edge0, highlight_edge1, highlight_edge2, highlight_edgeother);
  fprintf(fileout, "GEOMDOMAIN\n");
  fprintf(fileout, " %i %i\n", showgeom_inside_domain, showgeom_outside_domain);
  fprintf(fileout, "GEOMSHOW\n");
  fprintf(fileout, " %i %i %i %i %i %i %f %f\n",
     show_faces_interior, show_faces_exterior, show_faces_solid, show_faces_outline, smooth_geom_normal,
     geom_force_transparent, geom_transparency, geom_outline_width);
  fprintf(fileout, " %i %i %i %i\n", show_volumes_interior, show_volumes_exterior, show_volumes_solid, show_volumes_outline);
  fprintf(fileout, " %f %f\n", geom_vert_exag, geom_max_angle);

  fprintf(fileout, "GVERSION\n");
  fprintf(fileout, " %i\n", gversion);
  fprintf(fileout, "GVECDOWN\n");
  fprintf(fileout, " %i\n", gvec_down);
  fprintf(fileout, "HISTOGRAM\n");
  fprintf(fileout, " %i %i %i %i %i %f\n",
  histogram_static, histogram_nbuckets, histogram_show_numbers, histogram_show_graph, histogram_show_outline, histogram_width_factor);
  fprintf(fileout, "ISOTRAN2\n");
  fprintf(fileout, " %i\n", transparent_state);
  for(i = 0; i < nmeshes; i++){
    meshdata *meshi;

    meshi = meshinfo + i;
    if(meshi->mesh_offset_ptr != NULL){
      fprintf(fileout, "MESHOFFSET\n");
      fprintf(fileout, " %i\n", i);
    }
  }
  if(nmeshes>1){
    fprintf(fileout,"MESHVIS\n");
    fprintf(fileout," %i\n",nmeshes);

    for(i=0;i<nmeshes;i++){
      meshdata *meshi;

      meshi = meshinfo + i;
      fprintf(fileout," %i\n",meshi->blockvis);
    }
  }
  fprintf(fileout, "NORTHANGLE\n");
  fprintf(fileout, " %i\n", vis_northangle);
  fprintf(fileout, " %f %f %f\n", northangle_position[0], northangle_position[1], northangle_position[2]);
  fprintf(fileout, "OFFSETSLICE\n");
  fprintf(fileout, " %i\n", offset_slice);
  fprintf(fileout, "OUTLINEMODE\n");
  fprintf(fileout, " %i %i\n", highlight_flag, outline_color_flag);
  fprintf(fileout, "P3DSURFACETYPE\n");
  fprintf(fileout, " %i\n", p3dsurfacetype);
  fprintf(fileout, "P3DSURFACESMOOTH\n");
  fprintf(fileout, " %i\n", p3dsurfacesmooth);
  fprintf(fileout, "PROJECTION\n");
  fprintf(fileout, " %i\n", projection_type);
  fprintf(fileout, "SCALEDFONT\n");
  fprintf(fileout, " %i %f %i\n", scaled_font2d_height, scaled_font2d_height2width, scaled_font2d_thickness);
  fprintf(fileout, " %i %f %i\n", scaled_font3d_height, scaled_font3d_height2width, scaled_font3d_thickness);
  fprintf(fileout, "SHOWALLTEXTURES\n");
  fprintf(fileout, " %i\n", showall_textures);
  fprintf(fileout, "SHOWAXISLABELS\n");
  fprintf(fileout, " %i\n", visaxislabels);
  fprintf(fileout, "SHOWBLOCKLABEL\n");
  fprintf(fileout, " %i\n", visMeshlabel);
  fprintf(fileout, "SHOWBLOCKS\n");
  fprintf(fileout, " %i\n", visBlocks);
  fprintf(fileout, "SHOWCADANDGRID\n");
  fprintf(fileout, " %i\n", show_cad_and_grid);
  fprintf(fileout, "SHOWCADOPAQUE\n");
  fprintf(fileout, " %i\n", viscadopaque);
  fprintf(fileout, "SHOWCEILING\n");
  fprintf(fileout, " %i\n", visCeiling);
  fprintf(fileout, "SHOWCOLORBARS\n");
  fprintf(fileout, " %i\n", visColorbarVertical);
  fprintf(fileout, "SHOWCVENTS\n");
  fprintf(fileout, " %i %i\n", visCircularVents, circle_outline);
  fprintf(fileout, "SHOWDUMMYVENTS\n");
  fprintf(fileout, " %i\n", visDummyVents);
  fprintf(fileout, "SHOWEVACSLICES\n");
  fprintf(fileout, " %i %i %i\n", show_evac_slices, constant_evac_coloring, show_evac_colorbar);
  fprintf(fileout, "SHOWFLOOR\n");
  fprintf(fileout, " %i\n", visFloor);
  fprintf(fileout, "SHOWFRAME\n");
  fprintf(fileout, " %i\n", visFrame);
  fprintf(fileout, "SHOWFRAMELABEL\n");
  fprintf(fileout, " %i\n", visFramelabel);
  fprintf(fileout, "SHOWFRAMERATE\n");
  fprintf(fileout, " %i\n", visFramerate);
  fprintf(fileout, "SHOWGRID\n");
  fprintf(fileout, " %i\n", visGrid);
  fprintf(fileout, "SHOWGRIDLOC\n");
  fprintf(fileout, " %i\n", visgridloc);
  fprintf(fileout, "SHOWHMSTIMELABEL\n");
  fprintf(fileout, " %i\n", vishmsTimelabel);
  fprintf(fileout, "SHOWHRRCUTOFF\n");
  fprintf(fileout, " %i\n", show_hrrcutoff);
  fprintf(fileout, "SHOWHRRLABEL\n");
  fprintf(fileout, " %i\n", visHRRlabel);
  fprintf(fileout, "SHOWISO\n");
  fprintf(fileout, " %i\n", visAIso);
  fprintf(fileout, "SHOWISONORMALS\n");
  fprintf(fileout, " %i\n", show_iso_normal);
  fprintf(fileout, "SHOWLABELS\n");
  fprintf(fileout, " %i\n", visLabels);
#ifdef pp_memstatus
  fprintf(fileout, "SHOWMEMLOAD\n");
  fprintf(fileout, " %i\n", visAvailmemory);
#endif
  fprintf(fileout, "SHOWOPENVENTS\n");
  fprintf(fileout, " %i %i\n", visOpenVents, visOpenVentsAsOutline);
  fprintf(fileout, "SHOWOTHERVENTS\n");
  fprintf(fileout, " %i\n", visOtherVents);
  fprintf(fileout, "SHOWSENSORS\n");
  fprintf(fileout, " %i %i\n", visSensor, visSensorNorm);
  fprintf(fileout, "SHOWSLICEINOBST\n");
  fprintf(fileout, " %i\n", show_slice_in_obst);
  fprintf(fileout, "SHOWSMOKEPART\n");
  fprintf(fileout, " %i\n", visSmokePart);
  fprintf(fileout, "SHOWSPRINKPART\n");
  fprintf(fileout, " %i\n", visSprinkPart);
  fprintf(fileout, "SHOWSTREAK\n");
  fprintf(fileout, " %i %i %i %i\n", streak5show, streak5step, showstreakhead, streak_index);
  fprintf(fileout, "SHOWTERRAIN\n");
  fprintf(fileout, " %i\n", visTerrainType);
  fprintf(fileout, "SHOWTETRAS\n");
  fprintf(fileout, " %i %i\n", show_volumes_solid, show_volumes_outline);
  fprintf(fileout, "SHOWTHRESHOLD\n");
  fprintf(fileout, " %i %i %f\n", vis_threshold, vis_onlythreshold, temp_threshold);
  fprintf(fileout, "SHOWTICKS\n");
  fprintf(fileout, " %i\n", visFDSticks);
  fprintf(fileout, "SHOWTIMEBAR\n");
  fprintf(fileout, " %i %i\n", visTimebar,timebar_overlap);
  fprintf(fileout, "SHOWTIMELABEL\n");
  fprintf(fileout, " %i\n", visTimelabel);
  fprintf(fileout, "SHOWTITLE\n");
  fprintf(fileout, " %i\n", visTitle);
  fprintf(fileout, "SHOWCHID\n");
  fprintf(fileout, " %i\n", visCHID);
  fprintf(fileout, "SHOWTRACERSALWAYS\n");
  fprintf(fileout, " %i\n", show_tracers_always);
  fprintf(fileout, "SHOWTRIANGLES\n");
  fprintf(fileout, " %i %i %i %i 1 %i\n", show_iso_solid, show_iso_outline, show_iso_verts, show_iso_normal, smooth_iso_normal);
  fprintf(fileout, "SHOWTRANSPARENT\n");
  fprintf(fileout, " %i\n", visTransparentBlockage);
  fprintf(fileout, "SHOWTRANSPARENTVENTS\n");
  fprintf(fileout, " %i\n", show_transparent_vents);
  fprintf(fileout, "SHOWTRIANGLECOUNT\n");
  fprintf(fileout, " %i\n", show_triangle_count);
  fprintf(fileout, "SHOWVENTFLOW\n");
  fprintf(fileout, " %i %i %i %i %i\n", visVentHFlow, visventslab, visventprofile, visVentVFlow, visVentMFlow);
  fprintf(fileout, "SHOWROOMS\n");
  fprintf(fileout, " %i\n", visCompartments);
  fprintf(fileout, "SHOWVENTS\n");
  fprintf(fileout, " %i\n", visVents);
  fprintf(fileout, "SHOWWALLS\n");
  fprintf(fileout, " %i\n", visWalls);
  fprintf(fileout, "SKIPEMBEDSLICE\n");
  fprintf(fileout, " %i\n", skip_slice_in_embedded_mesh);
  fprintf(fileout, "SLICEDUP\n");
  fprintf(fileout, " %i %i %i\n", slicedup_option, vectorslicedup_option, boundaryslicedup_option);
  fprintf(fileout, "SMOKESENSORS\n");
  fprintf(fileout, " %i %i\n", show_smokesensors, test_smokesensors);
#ifdef pp_LANG
  fprintf(fileout, "STARTUPLANG\n");
  fprintf(fileout, " %s\n", startup_lang_code);
#endif
  fprintf(fileout, "STEREO\n");
  fprintf(fileout, " %i\n", stereotype);
  fprintf(fileout, "SURFINC\n");
  fprintf(fileout, " %i\n", surfincrement);
  fprintf(fileout, "TERRAINPARMS\n");
  fprintf(fileout, " %i %i %i\n", terrain_rgba_zmin[0], terrain_rgba_zmin[1], terrain_rgba_zmin[2]);
  fprintf(fileout, " %i %i %i\n", terrain_rgba_zmax[0], terrain_rgba_zmax[1], terrain_rgba_zmax[2]);
  fprintf(fileout, " %f\n", vertical_factor);
  fprintf(fileout, "TITLESAFE\n");
  fprintf(fileout, " %i\n", titlesafe_offset);
  if(trainer_mode == 1){
    fprintf(fileout, "TRAINERMODE\n");
    fprintf(fileout, " %i\n", trainer_mode);
  }
  fprintf(fileout, "TRAINERVIEW\n");
  fprintf(fileout, " %i\n", trainerview);
  fprintf(fileout, "TRANSPARENT\n");
  fprintf(fileout, " %i %f\n", use_transparency_data, transparent_level);
  fprintf(fileout, "TREEPARMS\n");
  fprintf(fileout, " %i %i %i %i\n", mintreesize,vis_xtree,vis_ytree,vis_ztree);
  fprintf(fileout, "TWOSIDEDVENTS\n");
  fprintf(fileout, " %i %i\n", show_bothsides_int, show_bothsides_ext);
  fprintf(fileout, "VECTORSKIP\n");
  fprintf(fileout, " %i\n", vectorskip);
  fprintf(fileout, "VOLSMOKE\n");
  fprintf(fileout, " %i %i %i %i %i\n",
    glui_compress_volsmoke, use_multi_threading, load_at_rendertimes, volbw, show_volsmoke_moving);
  fprintf(fileout, " %f %f %f %f %f %f %f\n",
    global_temp_min, global_temp_cutoff, global_temp_max, fire_opacity_factor, mass_extinct, gpu_vol_factor, nongpu_vol_factor
    );
  fprintf(fileout, "WINDROSEDEVICE\n");
  fprintf(fileout, " %i %i %i %i %i %i %i %i %i\n",
    viswindrose, showref_windrose, windrose_xy_vis, windrose_xz_vis, windrose_yz_vis, windstate_windrose, showlabels_windrose,
    windrose_first,windrose_next);
  fprintf(fileout, " %i %i %i %f %i %i\n", nr_windrose, ntheta_windrose, scale_windrose, radius_windrose, scale_increment_windrose, scale_max_windrose);
  {
    if(nwindrose_showhide > 0){
      int ii;

      UpdateWindRoseDevices(UPDATE_WINDROSE_SHOWHIDE);
      fprintf(fileout, "WINDROSESHOWHIDE\n");
      fprintf(fileout, " %i\n", nwindrose_showhide);
      for(ii = 0; ii < nwindrose_showhide; ii++){
        fprintf(fileout, " %i", windrose_showhide[ii]);
        if((ii+1)%WINDROSE_PER_ROW==0)fprintf(fileout, "\n");
      }
      fprintf(fileout, "\n");
    }
  }
  {
    float *xyzt;

    xyzt = windrose_merge_dxyzt;
    fprintf(fileout, "WINDROSEMERGE\n");
    fprintf(fileout, " %i %f %f %f %f\n",windrose_merge_type,xyzt[0],xyzt[1],xyzt[2],xyzt[3]);
  }
  fprintf(fileout, "ZOOM\n");
  fprintf(fileout, " %i %f\n", zoomindex, zoom);

  fprintf(fileout,"\n *** MISC ***\n\n");

  fprintf(fileout, "CELLCENTERTEXT\n");
  fprintf(fileout, " %i\n", cell_center_text);
  if(fds_filein != NULL&&strlen(fds_filein) > 0){
    fprintf(fileout, "INPUT_FILE\n");
    fprintf(fileout, " %s\n", fds_filein);
  }
  fprintf(fileout, "LABELSTARTUPVIEW\n");
  fprintf(fileout, " %s\n", startup_view_label);
  fprintf(fileout, "SHOWMESHMENUS\n");
  fprintf(fileout, " %i\n", show_meshmenus);
  fprintf(fileout, "RENDERCLIP\n");
  fprintf(fileout, " %i %i %i %i %i\n",
    clip_rendered_scene, render_clip_left, render_clip_right, render_clip_bottom, render_clip_top);
  fprintf(fileout, "RENDERFILELABEL\n");
  fprintf(fileout, " %i\n", render_label_type);
  fprintf(fileout, "RENDERFILETYPE\n");
  fprintf(fileout," %i %i\n",render_filetype,movie_filetype);
  fprintf(fileout, "MOVIEFILETYPE\n");
  fprintf(fileout," %i %i %i %i\n",movie_filetype,movie_framerate,movie_bitrate,quicktime_compatibility);
  if(nskyboxinfo>0){
    int iskybox;
    skyboxdata *skyi;
    char *filei;
    char *nullfile="NULL";

    for(iskybox=0;iskybox<nskyboxinfo;iskybox++){
      skyi = skyboxinfo + iskybox;
      fprintf(fileout,"SKYBOX\n");
      for(i=0;i<6;i++){
        filei = skyi->face[i].file;
        if(filei==NULL)filei=nullfile;
        if(strcmp(filei,"NULL")==0){
          fprintf(fileout,"NULL\n");
        }
        else{
          fprintf(fileout," %s\n",filei);
        }
      }
    }
  }
  fprintf(fileout, "RENDEROPTION\n");
  fprintf(fileout, " %i %i %i\n", render_window_size, resolution_multiplier, nheight360);
  fprintf(fileout, "UNITCLASSES\n");
  fprintf(fileout, " %i\n", nunitclasses);
  for(i = 0; i<nunitclasses; i++){
    fprintf(fileout, " %i\n", unitclasses[i].unit_index);
  }
  fprintf(fileout, "SHOWGRAVVECTOR\n");
  fprintf(fileout, " %i\n", showgravity_vector);
  if(zaxis_custom == 1){
    fprintf(fileout, "ZAXISANGLES\n");
    fprintf(fileout, " %f %f %f\n", zaxis_angles[0], zaxis_angles[1], zaxis_angles[2]);
  }

  fprintf(fileout,"\n *** 3D SMOKE INFO ***\n\n");

  fprintf(fileout,"ADJUSTALPHA\n");
  fprintf(fileout," %i\n",adjustalphaflag);
  {
    colorbardata *cb;
    char percen[2];

    cb = colorbarinfo + colorbartype;
    strcpy(percen, "%");
    fprintf(fileout, "COLORBARTYPE\n");
    fprintf(fileout, " %i %s %s \n", colorbartype, percen, cb->label);
  }
  {
    int mmin[3], mmax[3];
    for(i = 0; i < 3; i++){
      mmin[i] = rgb_below_min[i];
      mmax[i] = rgb_above_max[i];
    }
    fprintf(fileout, "EXTREMECOLORS\n");
    fprintf(fileout, " %i %i %i %i %i %i\n",
      mmin[0], mmin[1], mmin[2],
      mmax[0], mmax[1], mmax[2]);
  }
  fprintf(fileout, "FIRECOLOR\n");
  fprintf(fileout, " %i %i %i\n", fire_red, fire_green, fire_blue);
  fprintf(fileout, "FIRECOLORMAP\n");
  fprintf(fileout, " %i %i\n", firecolormap_type, fire_colorbar_index);
  fprintf(fileout, "FIREDEPTH\n");
  fprintf(fileout, " %f %f\n", fire_halfdepth, co2_halfdepth);
  if(ncolorbars > ndefaultcolorbars){
    colorbardata *cbi;
    unsigned char *rrgb;
    int n;

    fprintf(fileout, "GCOLORBAR\n");
    fprintf(fileout, " %i\n", ncolorbars - ndefaultcolorbars);
    for(n = ndefaultcolorbars; n < ncolorbars; n++){
      cbi = colorbarinfo + n;
      fprintf(fileout, " %s\n", cbi->label);
      fprintf(fileout, " %i %i\n", cbi->nnodes, cbi->nodehilight);
      for(i = 0; i < cbi->nnodes; i++){
        rrgb = cbi->rgb_node + 3 * i;
        fprintf(fileout, " %i %i %i %i\n", (int)cbi->index_node[i], (int)rrgb[0], (int)rrgb[1], (int)rrgb[2]);
      }
    }
  }
  fprintf(fileout, "SHOWEXTREMEDATA\n");
  {
    int show_extremedata = 0;

    if(show_extreme_mindata == 1 || show_extreme_maxdata == 1)show_extremedata = 1;
    fprintf(fileout, " %i %i %i\n", show_extremedata, show_extreme_mindata, show_extreme_maxdata);
  }
  fprintf(fileout, "SMOKECOLOR\n");
  fprintf(fileout, " %i %i %i\n", smoke_red, smoke_green, smoke_blue);
  fprintf(fileout, "SMOKECULL\n");
#ifdef pp_CULL
  fprintf(fileout," %i\n",cullsmoke);
#else
  fprintf(fileout," %i\n",smokecullflag);
#endif
  if(ABS(smoke_albedo - smoke_albedo_base) > 0.001){
    fprintf(fileout, "SMOKEALBEDO\n");
    fprintf(fileout, " %f\n", smoke_albedo);
  }
  fprintf(fileout, "SMOKESKIP\n");
  fprintf(fileout," %i\n",smokeskipm1);
#ifdef pp_GPU
  fprintf(fileout,"SMOKERTHICK\n");
  fprintf(fileout," %f\n",smoke3d_rthick);
#else
  fprintf(fileout,"SMOKETHICK\n");
  fprintf(fileout," %i\n",smoke3d_thick);
#endif
#ifdef pp_GPU
  fprintf(fileout, "USEGPU\n");
  fprintf(fileout, " %i\n", usegpu);
#endif
  fprintf(fileout, "VOLSMOKE\n");
  fprintf(fileout, " %i %i %i %i %i\n",
    glui_compress_volsmoke, use_multi_threading, load_at_rendertimes, volbw, show_volsmoke_moving);
  fprintf(fileout, " %f %f %f %f %f\n",
    global_temp_min, global_temp_cutoff, global_temp_max, fire_opacity_factor, mass_extinct);

  fprintf(fileout, "\n *** ZONE FIRE PARAMETRES ***\n\n");

  fprintf(fileout, "SHOWHAZARDCOLORS\n");
  fprintf(fileout, " %i\n", zonecolortype);
  fprintf(fileout, "SHOWHZONE\n");
  fprintf(fileout, " %i\n", visHZone);
  fprintf(fileout, "SHOWSZONE\n");
  fprintf(fileout, " %i\n", visSZone);
  fprintf(fileout, "SHOWVZONE\n");
  fprintf(fileout, " %i %i\n", visVZone, show_zonelower);
  fprintf(fileout, "SHOWZONEFIRE\n");
  fprintf(fileout, " %i\n", viszonefire);

  fprintf(fileout,"\n *** TOUR INFO ***\n\n");

  fprintf(fileout, "SHOWPATHNODES\n");
  fprintf(fileout, " %i\n", show_path_knots);
  fprintf(fileout, "SHOWTOURROUTE\n");
  fprintf(fileout, " %i\n", edittour);
  {
    float *col;

    fprintf(fileout, "TOURCOLORS\n");
    col = tourcol_selectedpathline;
    fprintf(fileout, " %f %f %f   :selected path line\n", col[0], col[1], col[2]);
    col = tourcol_selectedpathlineknots;
    fprintf(fileout, " %f %f %f   :selected path line knots\n", col[0], col[1], col[2]);
    col = tourcol_selectedknot;
    fprintf(fileout, " %f %f %f   :selected knot\n", col[0], col[1], col[2]);
    col = tourcol_pathline;
    fprintf(fileout, " %f %f %f   :path line\n", col[0], col[1], col[2]);
    col = tourcol_pathknots;
    fprintf(fileout, " %f %f %f   :path knots\n", col[0], col[1], col[2]);
    col = tourcol_text;
    fprintf(fileout, " %f %f %f   :text\n", col[0], col[1], col[2]);
    col = tourcol_avatar;
    fprintf(fileout, " %f %f %f   :avatar\n", col[0], col[1], col[2]);


  }
  fprintf(fileout, "TOURCONSTANTVEL\n");
  fprintf(fileout, " %i\n", tour_constant_vel);
  fprintf(fileout, "VIEWALLTOURS\n");
  fprintf(fileout, " %i\n", viewalltours);
  fprintf(fileout, "VIEWTIMES\n");
  fprintf(fileout, " %f %f %i\n", view_tstart, view_tstop, view_ntimes);
  fprintf(fileout, "VIEWTOURFROMPATH\n");
  fprintf(fileout," %i\n",viewtourfrompath);


  if(flag == LOCAL_INI)WriteIniLocal(fileout);
  if(
    ((INI_fds_filein != NULL&&fds_filein != NULL&&strcmp(INI_fds_filein, fds_filein) == 0) ||
    flag == LOCAL_INI))OutputViewpoints(fileout);

  {
    char version[256];
    char githash[256];
    char gitdate[256];

    GetProgVersion(version);
    GetGitInfo(githash,gitdate);    // get githash
    fprintf(fileout,"\n\n");
    fprintf(fileout,"# FDS/Smokeview Environment\n");
    fprintf(fileout,"# -------------------------\n\n");
    if(strcmp(version,"")!=0){
      fprintf(fileout,"# Smokeview Version: %s\n",version);
    }
    fprintf(fileout,"# Smokeview Build: %s\n",githash);
    fprintf(fileout,"# Smokeview Build Date: %s\n",__DATE__);
    if(fds_version!=NULL){
      fprintf(fileout,"# FDS Version: %s\n",fds_version);
    }
    if(fds_githash!=NULL){
      fprintf(fileout, "# FDS Build: %s\n", fds_githash);
    }
    fprintf(fileout,"# Platform: WIN64\n");
#ifdef pp_OSX
    PRINTF("Platform: OSX64\n");
#endif
#ifdef pp_LINUX
    fprintf(fileout,"# Platform: LINUX64\n");
#endif

    if(use_graphics==1){
      GLint nred, ngreen, nblue, ndepth, nalpha;

      glGetIntegerv(GL_RED_BITS,&nred);
      glGetIntegerv(GL_GREEN_BITS,&ngreen);
      glGetIntegerv(GL_BLUE_BITS,&nblue);
      glGetIntegerv(GL_DEPTH_BITS,&ndepth);
      glGetIntegerv(GL_ALPHA_BITS,&nalpha);
      fprintf(fileout, "\n\n");
      fprintf(fileout,"# Graphics Environment\n");
      fprintf(fileout,"# --------------------\n\n");
      if(use_graphics == 1){
        char version_label[256];
        char *glversion = NULL;
        char *glshadeversion = NULL;

        glversion = (char *)glGetString(GL_VERSION);
        if(glversion != NULL){
          strcpy(version_label, "OpenGL Version: ");
          strcat(version_label, glversion);
          fprintf(fileout, "# %s\n", version_label);
        }
        glshadeversion=(char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        if(glshadeversion != NULL){
          strcpy(version_label, "  GLSL Version: ");
          strcat(version_label, glshadeversion);
          fprintf(fileout, "# %s\n", version_label);
        }
      }
      fprintf(fileout, "#       Red bits:%i\n", nred);
      fprintf(fileout,"#     Green bits:%i\n",ngreen);
      fprintf(fileout,"#      Blue bits:%i\n",nblue);
      fprintf(fileout,"#     Alpha bits:%i\n",nalpha);
      fprintf(fileout,"#     Depth bits:%i\n\n",ndepth);
    }
  }

  if(fileout!=stdout)fclose(fileout);
}

/* ------------------ UpdateLoadedLists ------------------------ */

void UpdateLoadedLists(void){
  int i;
  slicedata *slicei;
  patchdata *patchi;

  nslice_loaded=0;
  for(i=0;i<nsliceinfo;i++){
    slicei = sliceinfo + i;
    if(slicei->loaded==1){
      slice_loaded_list[nslice_loaded]=i;
      nslice_loaded++;
    }
  }

  npatch_loaded=0;
  for(i=0;i<npatchinfo;i++){
    patchi = patchinfo + i;
    if(patchi->loaded==1){
      patch_loaded_list[npatch_loaded]=i;
      npatch_loaded++;
    }
  }

  nvolsmoke_loaded = 0;
  if(nvolrenderinfo>0){
    for(i=0;i<nmeshes;i++){
      meshdata *meshi;
      volrenderdata *vr;

      meshi = meshinfo + i;
      vr = &(meshi->volrenderinfo);
      if(vr==NULL||vr->fireslice==NULL||vr->smokeslice==NULL)continue;
      if(vr->loaded==0||vr->display==0)continue;
      nvolsmoke_loaded++;
    }
  }
}

/* ------------------ GetElevAz ------------------------ */

void GetElevAz(float *xyznorm,float *dtheta,float *rotate_axis, float *dpsi){

  // cos(dtheta) = (xyznorm .dot. vec3(0,0,1))/||xyznorm||
  // rotate_axis = xyznorm .cross. vec3(0,0,1)

  Normalize(xyznorm,3);
  *dtheta = RAD2DEG*acos(xyznorm[2]);
  rotate_axis[0]=-xyznorm[1];
  rotate_axis[1]= xyznorm[0];
  rotate_axis[2]=0.0;
  Normalize(rotate_axis,2);
  if(dpsi!=NULL){
    float xyznorm2[2];

    xyznorm2[0]=xyznorm[0];
    xyznorm2[1]=xyznorm[1];
    Normalize(xyznorm2,2);
    *dpsi = RAD2DEG*acos(xyznorm2[1]);
    if(xyznorm2[0]<0.0)*dpsi=-(*dpsi);
  }
}



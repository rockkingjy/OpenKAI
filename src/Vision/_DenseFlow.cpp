/*
 * _DenseFlow.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: yankai
 */

#include "_DenseFlow.h"

#ifdef USE_CUDA

namespace kai
{

_DenseFlow::_DenseFlow()
{
	m_pVision = NULL;
	m_pGrayFrames = NULL;
	m_w = 640;
	m_h = 480;
	m_bShowFlow = false;
}

_DenseFlow::~_DenseFlow()
{
}

bool _DenseFlow::init(void* pKiss)
{
	IF_F(!this->_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*)pKiss;
	pK->m_pInst = this;

	KISSm(pK,w);
	KISSm(pK,h);
	m_gFlow = GpuMat(m_h, m_w, CV_32FC2);

	m_pGrayFrames = new FrameGroup();
	m_pGrayFrames->init(2);
	m_pFarn = cuda::FarnebackOpticalFlow::create();
    m_mFlow = Mat::zeros(m_h, m_w, CV_32FC2);

	KISSm(pK,bShowFlow);

	return true;
}

bool _DenseFlow::link(void)
{
	IF_F(!this->_ThreadBase::link());
	Kiss* pK = (Kiss*)m_pKiss;

	string iName = "";
	F_INFO(pK->v("_VisionBase",&iName));
	m_pVision = (_VisionBase*)(pK->root()->getChildInstByName(&iName));

	return true;
}

bool _DenseFlow::start(void)
{
	NULL_T(m_pVision);

	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _DenseFlow::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		detect();

		this->autoFPSto();
	}
}

void _DenseFlow::detect(void)
{
	NULL_(m_pVision);
	Frame* pGray = m_pVision->Gray();
	NULL_(pGray);
	IF_(pGray->bEmpty());

	Frame* pNextFrame = m_pGrayFrames->getLastFrame();
	IF_(pGray->tStamp() <= pNextFrame->tStamp());

	Frame* pPrevFrame = m_pGrayFrames->getPrevFrame();
	m_pGrayFrames->updateFrameIndex();

	pNextFrame->copy(pGray->resize(m_w, m_h));
	GpuMat* pPrev = pPrevFrame->gm();
	GpuMat* pNext = pNextFrame->gm();

	IF_(pPrev->empty());
	IF_(pNext->empty());
	IF_(pPrev->size() != pNext->size());

	m_pFarn->calc(*pPrev, *pNext, m_gFlow);
	m_gFlow.download(m_mFlow);
}

vDouble2 _DenseFlow::vFlow(vDouble4* pROI)
{
	vDouble2 vF;
	vF.init();
	if(!pROI)return vF;

	vInt4 iR;
	iR.x = pROI->x * m_w;
	iR.y = pROI->y * m_h;
	iR.z = pROI->z * m_w;
	iR.w = pROI->w * m_h;

	if (iR.x < 0)
		iR.x = 0;
	if (iR.y < 0)
		iR.y = 0;
	if (iR.z >= m_w)
		iR.z = m_w - 1;
	if (iR.w >= m_h)
		iR.w = m_h - 1;

	return vFlow(&iR);
}

vDouble2 _DenseFlow::vFlow(vInt4* pROI)
{
	vDouble2 vF;
	vF.init();
	if(!pROI)return vF;

	int nX=0;
	int nY=0;
	for(int i=pROI->y; i<pROI->w; i++)
	{
		for(int j=pROI->x; j<pROI->z; j++)
		{
			Vec2f v = m_mFlow.at<Vec2f>(i,j);

			if(v.val[0] != 0.0)
			{
				vF.x += v.val[0];
				nX++;
			}

			if(v.val[1] != 0.0)
			{
				vF.y += v.val[1];
				nY++;
			}
		}
	}

	if(nX>0)vF.x /= (double)nX;
	if(nY>0)vF.y /= (double)nY;

	return vF;
}

bool _DenseFlow::draw(void)
{
	IF_F(!this->_ThreadBase::draw());
	Window* pWin = (Window*) this->m_pWindow;
	Frame* pFrame = pWin->getFrame();

	IF_T(!m_bShowFlow);
	IF_F(m_gFlow.empty());

    GpuMat planes[2];
    cuda::split(m_gFlow, planes);

    Mat flowx(planes[0]);
    Mat flowy(planes[1]);

    Mat out;
    drawOpticalFlow(flowx, flowy, out, 10);
    imshow(*this->getName(), out);

	return true;
}

bool _DenseFlow::isFlowCorrect(Point2f u)
{
    return !cvIsNaN(u.x) && !cvIsNaN(u.y) && fabs(u.x) < 1e9 && fabs(u.y) < 1e9;
}

Vec3b _DenseFlow::computeColor(float fx, float fy)
{
    static bool first = true;

    // relative lengths of color transitions:
    // these are chosen based on perceptual similarity
    // (e.g. one can distinguish more shades between red and yellow
    //  than between yellow and green)
    const int RY = 15;
    const int YG = 6;
    const int GC = 4;
    const int CB = 11;
    const int BM = 13;
    const int MR = 6;
    const int NCOLS = RY + YG + GC + CB + BM + MR;
    static Vec3i colorWheel[NCOLS];

    if (first)
    {
        int k = 0;

        for (int i = 0; i < RY; ++i, ++k)
            colorWheel[k] = Vec3i(255, 255 * i / RY, 0);

        for (int i = 0; i < YG; ++i, ++k)
            colorWheel[k] = Vec3i(255 - 255 * i / YG, 255, 0);

        for (int i = 0; i < GC; ++i, ++k)
            colorWheel[k] = Vec3i(0, 255, 255 * i / GC);

        for (int i = 0; i < CB; ++i, ++k)
            colorWheel[k] = Vec3i(0, 255 - 255 * i / CB, 255);

        for (int i = 0; i < BM; ++i, ++k)
            colorWheel[k] = Vec3i(255 * i / BM, 0, 255);

        for (int i = 0; i < MR; ++i, ++k)
            colorWheel[k] = Vec3i(255, 0, 255 - 255 * i / MR);

        first = false;
    }

    const float rad = sqrt(fx * fx + fy * fy);
    const float a = atan2(-fy, -fx) / (float) CV_PI;

    const float fk = (a + 1.0f) / 2.0f * (NCOLS - 1);
    const int k0 = static_cast<int>(fk);
    const int k1 = (k0 + 1) % NCOLS;
    const float f = fk - k0;

    Vec3b pix;

    for (int b = 0; b < 3; b++)
    {
        const float col0 = colorWheel[k0][b] / 255.0f;
        const float col1 = colorWheel[k1][b] / 255.0f;

        float col = (1 - f) * col0 + f * col1;

        if (rad <= 1)
            col = 1 - rad * (1 - col); // increase saturation with radius
        else
            col *= .75; // out of range

        pix[2 - b] = static_cast<uchar>(255.0 * col);
    }

    return pix;
}

void _DenseFlow::drawOpticalFlow(const Mat_<float>& flowx, const Mat_<float>& flowy, Mat& dst, float maxmotion)
{
    dst.create(flowx.size(), CV_8UC3);
    dst.setTo(Scalar::all(0));

    // determine motion range:
    float maxrad = maxmotion;

    if (maxmotion <= 0)
    {
        maxrad = 1;
        for (int y = 0; y < flowx.rows; ++y)
        {
            for (int x = 0; x < flowx.cols; ++x)
            {
                Point2f u(flowx(y, x), flowy(y, x));

                if (!isFlowCorrect(u))
                    continue;

                maxrad = max(maxrad, sqrt(u.x * u.x + u.y * u.y));
            }
        }
    }

    for (int y = 0; y < flowx.rows; ++y)
    {
        for (int x = 0; x < flowx.cols; ++x)
        {
            Point2f u(flowx(y, x), flowy(y, x));

            if (isFlowCorrect(u))
                dst.at<Vec3b>(y, x) = computeColor(u.x / maxrad, u.y / maxrad);
        }
    }
}

}
#endif
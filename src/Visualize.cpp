//------------------------------------------------------------------------------
// <copyright file="Visualize.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include <stdafx.h>
#include <FaceTrackLib.h>
#include <math.h>


HRESULT VisualizeFacetracker(IFTImage* pColorImg, IFTResult* pAAMRlt, UINT32 color)
{
    if (!pColorImg->GetBuffer() || !pAAMRlt)
    {
        return E_POINTER;
    }

    // Insufficient data points to render face data
    FT_VECTOR2D* pPts2D;
    UINT pts2DCount;
    HRESULT hr = pAAMRlt->Get2DShapePoints(&pPts2D, &pts2DCount);
    if (FAILED(hr))
    {
        return hr;
    }

    if (pts2DCount < 86)
    {
        return E_INVALIDARG;
    }


    POINT* pFaceModel2DPoint = reinterpret_cast<POINT*>(_malloca(sizeof(POINT) * pts2DCount));
    if (!pFaceModel2DPoint)
    {
        return E_OUTOFMEMORY;
    }


    for (UINT ipt = 0; ipt < pts2DCount; ++ipt)
    {
        pFaceModel2DPoint[ipt].x = LONG(pPts2D[ipt].x + 0.5f);
        pFaceModel2DPoint[ipt].y = LONG(pPts2D[ipt].y + 0.5f);
    }

    for (UINT ipt = 0; ipt < 8; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[(ipt+1)%8];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 8; ipt < 16; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[(ipt-8+1)%8+8];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 16; ipt < 26; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[(ipt-16+1)%10+16];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 26; ipt < 36; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[(ipt-26+1)%10+26];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 36; ipt < 47; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[ipt+1];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 48; ipt < 60; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[(ipt-48+1)%12+48];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 60; ipt < 68; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[(ipt-60+1)%8+60];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }

    for (UINT ipt = 68; ipt < 86; ++ipt)
    {
        POINT ptStart = pFaceModel2DPoint[ipt];
        POINT ptEnd = pFaceModel2DPoint[ipt+1];
        pColorImg->DrawLine(ptStart, ptEnd, color, 1);
    }
    _freea(pFaceModel2DPoint);

    return hr;
}

HRESULT VisualizeFaceModel(IFTImage* pColorImg, IFTModel* pModel, FT_CAMERA_CONFIG const* pCameraConfig, FLOAT const* pSUCoef, 
    FLOAT zoomFactor, POINT viewOffset, IFTResult* pAAMRlt, UINT32 color)
{
    if (!pColorImg || !pModel || !pCameraConfig || !pSUCoef || !pAAMRlt)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    UINT vertexCount = pModel->GetVertexCount();
    FT_VECTOR2D* pPts2D = reinterpret_cast<FT_VECTOR2D*>(_malloca(sizeof(FT_VECTOR2D) * vertexCount));
    if (pPts2D)
    {
        FLOAT *pAUs;
        UINT auCount;
        hr = pAAMRlt->GetAUCoefficients(&pAUs, &auCount);
        if (SUCCEEDED(hr))
        {
            FLOAT scale, rotationXYZ[3], translationXYZ[3];
            hr = pAAMRlt->Get3DPose(&scale, rotationXYZ, translationXYZ);
            if (SUCCEEDED(hr))
            {
                hr = pModel->GetProjectedShape(pCameraConfig, zoomFactor, viewOffset, pSUCoef, pModel->GetSUCount(), pAUs, auCount, 
                    scale, rotationXYZ, translationXYZ, pPts2D, vertexCount);
                if (SUCCEEDED(hr))
                {
                    POINT* p3DMdl   = reinterpret_cast<POINT*>(_malloca(sizeof(POINT) * vertexCount));
                    if (p3DMdl)
                    {
                        for (UINT i = 0; i < vertexCount; ++i)
                        {
                            p3DMdl[i].x = LONG(pPts2D[i].x + 0.5f);
                            p3DMdl[i].y = LONG(pPts2D[i].y + 0.5f);
                        }

                        FT_TRIANGLE* pTriangles;
                        UINT triangleCount;
                        hr = pModel->GetTriangles(&pTriangles, &triangleCount);
                        if (SUCCEEDED(hr))
                        {
                            struct EdgeHashTable
                            {
                                UINT32* pEdges;
                                UINT edgesAlloc;

                                void Insert(int a, int b) 
                                {
                                    UINT32 v = (min(a, b) << 16) | max(a, b);
                                    UINT32 index = (v + (v << 8)) * 49157, i;
                                    for (i = 0; i < edgesAlloc - 1 && pEdges[(index + i) & (edgesAlloc - 1)] && v != pEdges[(index + i) & (edgesAlloc - 1)]; ++i)
                                    {
                                    }
                                    pEdges[(index + i) & (edgesAlloc - 1)] = v;
                                }
                            } eht;

                            eht.edgesAlloc = 1 << UINT(log(2.f * (1 + vertexCount + triangleCount)) / log(2.f));
                            eht.pEdges = reinterpret_cast<UINT32*>(_malloca(sizeof(UINT32) * eht.edgesAlloc));
                            if (eht.pEdges)
                            {
                                ZeroMemory(eht.pEdges, sizeof(UINT32) * eht.edgesAlloc);
                                for (UINT i = 0; i < triangleCount; ++i)
                                { 
                                    eht.Insert(pTriangles[i].i, pTriangles[i].j);
                                    eht.Insert(pTriangles[i].j, pTriangles[i].k);
                                    eht.Insert(pTriangles[i].k, pTriangles[i].i);
                                }
                                for (UINT i = 0; i < eht.edgesAlloc; ++i)
                                {
                                    if(eht.pEdges[i] != 0)
                                    {
                                        pColorImg->DrawLine(p3DMdl[eht.pEdges[i] >> 16], p3DMdl[eht.pEdges[i] & 0xFFFF], color, 1);
                                    }
                                }
                                _freea(eht.pEdges);
                            }

                            // Render the face rect in magenta
                            RECT rectFace;
                            hr = pAAMRlt->GetFaceRect(&rectFace);
                            if (SUCCEEDED(hr))
                            {
                                POINT leftTop = {rectFace.left, rectFace.top};
                                POINT rightTop = {rectFace.right - 1, rectFace.top};
                                POINT leftBottom = {rectFace.left, rectFace.bottom - 1};
                                POINT rightBottom = {rectFace.right - 1, rectFace.bottom - 1};
                                UINT32 nColor = 0xff00ff;
                                SUCCEEDED(hr = pColorImg->DrawLine(leftTop, rightTop, nColor, 1)) &&
                                    SUCCEEDED(hr = pColorImg->DrawLine(rightTop, rightBottom, nColor, 1)) &&
                                    SUCCEEDED(hr = pColorImg->DrawLine(rightBottom, leftBottom, nColor, 1)) &&
                                    SUCCEEDED(hr = pColorImg->DrawLine(leftBottom, leftTop, nColor, 1));
                            }
                        }

                        _freea(p3DMdl); 
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        _freea(pPts2D);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}




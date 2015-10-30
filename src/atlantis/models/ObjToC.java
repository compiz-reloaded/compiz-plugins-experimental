/*
 * ObjToC.java
 *
 * This file was used to generate all the c code to display the models.
 * A configuration file stores necessary details about the file being
 * processed. See the README file for more details.
 *
 *
 * Copyright : (C) 2007-2008 by David Mikos
 * Email     : infiniteloopcounter@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.StringTokenizer;
import java.util.ArrayList;
import java.util.Arrays;

public class ObjToC {

	static String openGLinit = "    glRotatef(180.0, 0.0, 1.0, 0.0);\n"+
	"    glRotatef(90.0, 0.0, 1.0, 0.0);\n"+
	"    glEnable(GL_CULL_FACE);";

	static String openGLfin  = "    glDisable(GL_CULL_FACE);";
							   //"    glEnable(GL_DEPTH_TEST);";

	static String glRenderSide = "GL_FRONT";


	static boolean ignoreMaterials = false;
	static boolean copyDiffuseToAmbient = true;
	static boolean wire = true;
	static boolean setCullFace = true;
	static boolean separateObjects = true;
	static boolean labelSeparateObjects = true;

	static float ambientFactor = 0.8f;
	static float diffuseFactor = 0.8f;
	static float specularFactor = 0.1f;
	static float shininessFactor = 1.0f;



	static boolean wave = false;
	static boolean linear = true; //otherwise do cubic interpolation between frames


	static float xOffset = 0.0f;
	static float amplitude = 1.0f;
	static float frequencyFactor = 0.25f;

//	other things
	static float[][][] pts;
	static float[][][] texture;
	static float[][][] normal;

	static boolean[][] changeInPts;
	static boolean[][] changeInNormals;

	static ArrayList<int[]> indices;	//(pt, texture, normal) indices
	static int[] orderedIndices;		//the indices order from (f)ace ordering

	static ArrayList<int[]> groups;		// (# sides, # polygons with same # sides)


	static ArrayList<boolean[]> materialAttributes;
	static ArrayList<String> materialNames;
	static ArrayList<String> objectNames;

	static int vCounter = 0, vtCounter = 0, vnCounter = 0, fCounter = 0;
	static int aniCounter = 0;
	static int fileCounter = 0;

	static boolean initDrawingRoutine = false;
	static boolean initAnimateRoutine = false;
	static boolean initVertexArray    = false;

	static String[] attributes = { "shininess", "ambient", "diffuse", "specular"};
	static String[][] attr;
	static String transparency = null;
	static String currentMaterial = "";
	static boolean newMaterial = false;
	static boolean disableColorMaterial = false;
	static boolean twoSided = false;
	static int prevSides = 0;
	static int sides = 0;



	static String filename = "";
	static String baseFilename = "";
	static String methodName = "";
	static String post = "";
	static String outputFolder = "";
	static String headerIntro = "";
	static String sourceIntro = "";
	static float scaleFactor = 1;
	static boolean numberedAnimation = false;
	static int maxNumZeros = 0;
	static int startFrame = 0;

	static String indent  = "    ";
	static String indent2 = "\t";
	static PrintStream out;



	private static final void print (String s)   { System.out.print (s); }
	private static final void println (String s) { System.out.println (s); }
	private static final void println ()         { System.out.println (); }

	public static void main(String[] args) throws IOException
	{
		if (args.length==0)
		{
			println ("Usage: java objToC [file1.config ...]");
			println ("See README for details on configuration file.");
		}

		for (int i=0; i<args.length; i++)
		{
			loadConfig (args[i]);
			if (filename == null)
				continue;

			checkFiles();

			if (fileCounter == 0)
				continue;

			out = new PrintStream(new FileOutputStream(outputFolder+baseFilename+".h"));
			if (!headerIntro.equals(""))
				out.println (headerIntro);

			materialNames = new ArrayList<String>();
			materialAttributes = new ArrayList<boolean[]>();
			attr= new String[4][4];

			objectNames = new ArrayList<String>();

			loadMtlFile();

			checkObjFile();
			pts    = new float[fileCounter][vCounter][3];
			texture= new float[fileCounter][vtCounter][2];
			normal = new float[fileCounter][vnCounter][3];

			changeInPts     = new boolean[vCounter][3];
			changeInNormals = new boolean[vnCounter][3];

			indices = new ArrayList<int[]>();
			orderedIndices = new int[fCounter];

			groups = new ArrayList<int[]>();


			fillVerticesNormalsObjFile();
			fillIndicesObjFile();

			printVerticesNormalsIndicesObjFile();

			println ("Written "+outputFolder+baseFilename+".h");

            out = new PrintStream(new FileOutputStream(outputFolder+baseFilename+".c"));

            out.print (sourceIntro);
            out.println ("#include \""+baseFilename+".h\"");

			animateObjFile();
			vertexArrayObjFile();
			initEndObjFile();

			println ("Written "+outputFolder+baseFilename+".c");
		}
	}

	public static void loadConfig (String configFile) throws IOException
	{
		//set defaults
		filename = "";
		outputFolder = "";
		numberedAnimation = false;
		methodName = "";
		scaleFactor = 1;
		headerIntro = "";
		sourceIntro = "";

		wave = false;
		amplitude = 1;
		frequencyFactor = 1;
		xOffset = 0;

		FileInputStream fis;
		BufferedReader br;

		try
		{
			fis = new FileInputStream(configFile);
			br = new BufferedReader(new InputStreamReader(fis));
		}
		catch (IOException e)
		{
			System.err.println (e);
			filename = null;
			return;
		}

		String input;
		String s;

		while ((input = br.readLine()) != null)
		{
			StringTokenizer st = new StringTokenizer(input, " \t");
			if (st.countTokens()!=1)
				continue;

			s = st.nextToken();
			if (s.charAt(0) != '@')
				continue;

			if (s.equals("@filename"))
			{
				if ((input = br.readLine()) == null)
					break;
				filename = input;
			}
			else if (s.equals("@outputFolder"))
			{
				if ((input = br.readLine()) == null)
					break;
				outputFolder = input;
				if (outputFolder.equals(""))
					continue;

				if (outputFolder.charAt(outputFolder.length()-1) != '/')
					outputFolder += "/";
			}
			else if (s.equals("@numberedAnimation"))
			{
				if ((input = br.readLine()) == null)
					break;

				st = new StringTokenizer(input, " \t");
				if (st.countTokens()!=1)
					continue;

				numberedAnimation = Boolean.parseBoolean(st.nextToken());
			}
			else if (s.equals("@methodName"))
			{
				if ((input = br.readLine()) == null)
					break;

				st = new StringTokenizer(input, " \t");
				if (!st.hasMoreTokens())
					continue;

				methodName = st.nextToken();
			}
			else if (s.equals("@waveEffect"))
			{
				if ((input = br.readLine()) == null)
					break;

				st = new StringTokenizer(input, " \t");
				if (st.countTokens()<3)
					continue;

				amplitude       = Float.parseFloat(st.nextToken());
				frequencyFactor = Float.parseFloat(st.nextToken());
				xOffset         = Float.parseFloat(st.nextToken());

				wave = true;
			}
			else if (s.equals("@scaleFactor"))
			{
				if ((input = br.readLine()) == null)
					break;

				st = new StringTokenizer(input, " \t");
				if (st.countTokens()!=1)
					continue;

				scaleFactor = Float.parseFloat(st.nextToken());
			}
			else if (s.equals("@headerIntro"))
			{
				headerIntro = "";
				while ((input = br.readLine()) != null)
				{
					st = new StringTokenizer(input, " \t");
					if (st.countTokens()==1)
						if (st.nextToken().equals("headerIntro@"))
							break;
					headerIntro += input+"\n";
				}
			}
			else if (s.equals("@sourceIntro"))
			{
				sourceIntro = "";
				while ((input = br.readLine()) != null)
				{
					st = new StringTokenizer(input, " \t");
					if (st.countTokens()==1)
						if (st.nextToken().equals("sourceIntro@"))
							break;
					sourceIntro += input+"\n";
				}
			}
		}

		if (numberedAnimation)
			wave = false;
	}

	static void checkFiles() throws IOException {
		fileCounter = 0;

		boolean lastCharANumber = false;


		int start = filename.length();
		int end   = filename.length();

		for (int i=0; i<filename.length(); i++)
		{
			char c = filename.charAt(i);
			if (c >= '0' && c <='9')
			{
				if (!lastCharANumber)
					start = i;

				end = i+1;
				lastCharANumber=true;
			}
			else
			{
				lastCharANumber = false;
			}
		}

		startFrame  = 0;
		fileCounter = 0;

		if (filename.equals(""))
		{
			System.err.println ("You need to specify an \"obj\" file in the configuration.");
			return;
		}

		if (filename.length()<4 ||
			!filename.substring(filename.length()-4, filename.length()).equalsIgnoreCase(".obj"))
		{
			System.err.println ("File is not of type \"obj\".");
			return;
		}

		if (numberedAnimation)
		{
			post = filename.substring(end, filename.length());
			startFrame = Integer.parseInt(filename.substring(start, end));
			filename = filename.substring(0, start);
			maxNumZeros = end - start;
		}

		baseFilename = (numberedAnimation ? filename : filename.substring(0, filename.length()-4));

        int index = baseFilename.lastIndexOf('/');

        if (index >= 0)
		baseFilename = baseFilename.substring(index+1, baseFilename.length());

        if (methodName.equals(""))
		methodName = baseFilename.substring(0,1).toUpperCase() + baseFilename.substring(1);

		try {
			do{
				new FileInputStream(getFullFilename(filename,fileCounter+startFrame));
				println ("Opened "+ getFullFilename(filename,fileCounter+startFrame));

				fileCounter++;
			} while (numberedAnimation);
		}
		catch (IOException e) {
			if (fileCounter == 0)
				System.err.println ("Cannot open "+getFullFilename(filename, startFrame));
		}
	}

	static String getFullFilename(String filename, int fc) {
		if (numberedAnimation)
			return filename + addLeadingZeros(fc, maxNumZeros) + post;

		return filename;
	}

	static String addLeadingZeros(int x, int maxZeros) {
		String s = "";

		int j=1;
		for (int i=1; i<maxZeros; i++) {
			j*=10;
			if (x<j) s+="0";
		}
		return s+x;
	}

	static void loadMtlFile() throws IOException {

		String materialFile = getFullFilename(filename, startFrame);
		materialFile = materialFile.substring(0, materialFile.length()-3)+"mtl";

		println ("Opened "+materialFile);

		FileInputStream fis;
		BufferedReader br;

		try {
			fis = new FileInputStream(materialFile);
			br = new BufferedReader(new InputStreamReader(fis));
		}
		catch (IOException e) {
			return;
		}

		newMaterial = false;
		transparency = "1.0000";

		for (int i=0; i<attr.length; i++) {
			for (int j=0; j<attr[0].length; j++) {
				attr[i][j]=null;
			}
		}

		String input;
		while ((input=br.readLine())!=null) {
			StringTokenizer st = new StringTokenizer(input, " \t");

			if (!st.hasMoreTokens()) continue;

			String s = st.nextToken();
			if (s.equals("newmtl")) {
				String mat = "";
				while (st.hasMoreTokens()) {
					mat += st.nextToken(" \t.,;:+-*/\\=!?\"'[](){}|");
				}
				if (mat.equals("")) continue;

				newAttributes();

				currentMaterial = mat;

				boolean[] b = new boolean[4];
				materialNames.add(mat);

				for (int i=0; i<b.length; i++) {
					b[i]=false;
				}
				materialAttributes.add(b);

				newMaterial = true;
			}
			else if (s.equals("Ns")) {
				if (!st.hasMoreTokens() || currentMaterial.equals("")) continue;

				attr[0][0]=st.nextToken();
			}
			else if (s.equals("Ka")) {
				if (st.countTokens()<3 || currentMaterial.equals("")) continue;

				attr[1][0]=st.nextToken();
				attr[1][1]=st.nextToken();
				attr[1][2]=st.nextToken();
				if (st.hasMoreTokens()) attr[1][3]=st.nextToken();
			}
			else if (s.equals("Kd")) {
				if (st.countTokens()<3 || currentMaterial.equals("")) continue;

				attr[2][0]=st.nextToken();
				attr[2][1]=st.nextToken();
				attr[2][2]=st.nextToken();
				if (st.hasMoreTokens()) attr[2][3]=st.nextToken();
			}
			else if (s.equals("Ks")) {
				if (st.countTokens()<3 || currentMaterial.equals("")) continue;
				attr[3][0]=st.nextToken();
				attr[3][1]=st.nextToken();
				attr[3][2]=st.nextToken();
				if (st.hasMoreTokens()) attr[3][3]=st.nextToken();
			}
			else if (s.equals("d") || s.equals("Tr")) {
				if (!st.hasMoreTokens() || currentMaterial.equals("")) continue;

				transparency = st.nextToken();
			}
		}
		newAttributes();
	}

	static void newAttributes() {
		if (!newMaterial) return;

		if (copyDiffuseToAmbient)
		{
			attr[1][0] = attr[2][0];
			attr[1][1] = attr[2][1];
			attr[1][2] = attr[2][2];
			attr[1][3] = attr[2][3];
		}

		attr[0][0] = ""+(Float.parseFloat(attr[0][0])*shininessFactor);
		for (int i=0; i<3; i++)
		{
			attr[1][i] = ""+(Float.parseFloat(attr[1][i])*ambientFactor);
			attr[2][i] = ""+(Float.parseFloat(attr[2][i])*diffuseFactor);
			attr[3][i] = ""+(Float.parseFloat(attr[3][i])*specularFactor);
		}

		Boolean userDefined = false;
		if (currentMaterial.substring(0,Math.min("userDefined".length(),currentMaterial.length())).equalsIgnoreCase("userDefined"))
			userDefined = true;


		for (int i=0; i<4; i++) {
			if (attr[i][0]!=null)
				materialAttributes.get(materialAttributes.size()-1)[i]=true;

			out.print ("static "+(userDefined ? "": "const ")+"GLfloat "+currentMaterial+"_"+attributes[i]+"[] = {" +attr[i][0]);
			if (i!=0) {
				out.print (","+attr[i][1]+","+attr[i][2]+",");
				if (attr[i][3]==null) out.print(transparency);
				else out.print (attr[i][3]);
			}
			out.println ("};");
		}
		materialAttributes.get(materialAttributes.size()-1)[0]=true;

		for (int i=0; i<attr.length; i++) {
			for (int j=0; j<attr[0].length; j++) {
				attr[i][j]=null;
			}
		}
		out.println();
	}



	static void checkObjFile () throws IOException {
		FileInputStream fis = new FileInputStream(getFullFilename(filename, startFrame));
		BufferedReader br = new BufferedReader(new InputStreamReader(fis));

		vCounter = 0; vtCounter = 0; vnCounter = 0; fCounter = 0;

		String input;
		while ((input=br.readLine())!=null) {
			StringTokenizer st = new StringTokenizer(input);
			if (!st.hasMoreTokens()) continue;

			String s = st.nextToken();
			if (s.equalsIgnoreCase("v")) vCounter++;
			else if (s.equalsIgnoreCase("vt")) vtCounter++;
			else if (s.equalsIgnoreCase("vn")) vnCounter++;
			else if (s.equalsIgnoreCase("f")) {

				for (; st.hasMoreTokens(); fCounter++)
					st.nextToken();
			}
		}
		br.close();
		fis.close();
	}

	static void fillVerticesNormalsObjFile () throws IOException {

		for (int j=0; j<3; j++)
		{
			for (int i=0; i<vCounter; i++)
				changeInPts[i][j] = false;

			for (int i=0; i<vnCounter; i++)
				changeInNormals[i][j] = false;
		}

		for (int j=0; j<fileCounter; j++) {
			FileInputStream fis = new FileInputStream(getFullFilename(filename,j+startFrame));
			BufferedReader br = new BufferedReader(new InputStreamReader(fis));

			vCounter = 0; vtCounter=0; vnCounter = 0;

			String input;
			while ((input=br.readLine())!=null) {
				StringTokenizer st = new StringTokenizer(input, " \t");

				if (!st.hasMoreTokens()) continue;
				String s = st.nextToken();
				if (s.equalsIgnoreCase("v")) {

					float [] rt = new float[3];
					for (int i=0; i<3; i++) {
						s = st.nextToken();
						rt[i] = Float.parseFloat(s)*scaleFactor;
					}

					pts[j][vCounter] = new float[] { rt[0], rt[1], rt[2] }; // y,z,x

					if (j!=0)
						for (int i=0; i<3; i++)
						{
							if (pts[j][vCounter][i] != pts[(j-1)][vCounter][i])
								changeInPts[vCounter][i] = true;
						}

					vCounter++;

				}
				if (s.equalsIgnoreCase("vt")) {

					float [] rt = new float[2];
					for (int i=0; i<2; i++) {
						s = st.nextToken();
						rt[i] = Float.parseFloat(s);
					}

					texture[j][vtCounter] = new float[] { rt[0], rt[1] };
					vtCounter++;

				}
				else if (s.equalsIgnoreCase("vn")) {
					float [] rt = new float[3];
					float sumSquares = 0;
					for (int i=0; i<3; i++) {
						s = st.nextToken();
						rt[i] = Float.parseFloat(s);
						sumSquares+=rt[i]*rt[i];
					}

					if (sumSquares==0) sumSquares = 1;

					for (int i=0; i<3; i++) {
						//rt[i]/=sumSquares;
					}

					normal[j][vnCounter] = new float[] { rt[0], rt[1], rt[2] }; // y,z,x

					if (j!=0)
						for (int i=0; i<3; i++)
						{
							if (normal[j][vnCounter][i] != normal[(j-1)][vnCounter][i])
								changeInPts[vnCounter][i] = true;
						}

					vnCounter++;
				}

			}
			br.close();
			fis.close();
		}
	}
	static void fillIndicesObjFile () throws IOException {

		for (int j=0; j<1; j++) {
			FileInputStream fis = new FileInputStream(getFullFilename(filename,j+startFrame));
			BufferedReader br = new BufferedReader(new InputStreamReader(fis));

			fCounter = 0;
			int distinctFCounter = 0;

			prevSides = 0;
			sides = 0;

			Boolean prevSpecifiedNormal = false;
			Boolean specifiedNormal = false;

			Boolean prevSpecifiedTexture = false;
			Boolean specifiedTexture = false;

			Boolean newObject = false;

			int objectCounter = -1;
			int materialIndex = -1;
			int prevMaterialIndex = -1;

			String input;
			while ((input=br.readLine())!=null) {
				StringTokenizer st = new StringTokenizer(input);

				if (!st.hasMoreTokens()) continue;
				String s = st.nextToken();
				if (s.equalsIgnoreCase("f")) {

					sides = 0;
					specifiedNormal = true;

					nextElement:

					while (st.hasMoreTokens())
					{

						int[] array = { -1, -1, -1 };

						StringTokenizer st2 = new StringTokenizer(st.nextToken(),"/",true);
						for (int i=0; i<3 && st2.hasMoreTokens(); i++)
						{
							s = st2.nextToken();

							if (s.equals("/")) continue;

							array[i] = Integer.parseInt(s)-1;

							if (st2.hasMoreTokens())
								s = st2.nextToken();

						}
						if (array[1]==-1) specifiedTexture = false;
						if (array[2]==-1) specifiedNormal  = false;

						for (int i=0; i<indices.size(); i++)
						{
							if (Arrays.equals(array,indices.get(i)))
							{
								orderedIndices[fCounter] = i;
								fCounter++; sides++;
								continue nextElement;
							}
						}
						indices.add(array);

						orderedIndices[fCounter] = distinctFCounter;
						fCounter++; distinctFCounter++; sides++;
					}

					//group indices by side, #, (object), (material)
					if (groups.size()==0 || prevSides!=sides ||
							prevSpecifiedNormal !=specifiedNormal ||
							prevSpecifiedTexture != specifiedTexture ||
							sides>4 ||
							(newObject && separateObjects) ||
							materialIndex!=prevMaterialIndex)
					{
						prevSides = sides;
						prevSpecifiedNormal = specifiedNormal;
						prevMaterialIndex = materialIndex;

						groups.add(new int[] { sides, 1, objectCounter, materialIndex });
					}
					else
					{
						groups.get(groups.size()-1)[1]+=1;
					}

					newObject = false;
				}
				else if (s.equalsIgnoreCase("o")) {
					newObject = true;
					objectCounter++;
					objectNames.add(st.nextToken());
				}
				else if (s.equalsIgnoreCase("usemtl")) {
					s = st.nextToken();
					for (int i=0; i<materialNames.size(); i++)
					{
						if (s.equals(materialNames.get(i)))
						{
							materialIndex = i;
							break;
						}
					}
				}
			}
			br.close();
			fis.close();
		}
	}


	static void printVerticesNormalsIndicesObjFile () {

		int size = indices.size();
		for (int k=0; k<3; k++)
		{
			if (k==1 && vtCounter==0) continue;
			if (k==2 && vnCounter==0 ) continue;


			out.print("static float "+methodName);
			if (k==0) out.println ("Points["+size*3+"] = {");
			else if (k==1) out.println("Texture["+size*2+"] = {");
			else if (k==2) out.println ("Normals["+size*3+"] = {");
			for (int i=0; i<size; i++)
			{
				out.print(indent);
				for (int j=0; j<(k==1 ? 2: 3); j++)
				{
					int ind = indices.get(i)[k];
					if (ind>=0) {
						if (k==0)      out.print(    pts[0][ind][j]);
						else if (k==1) out.print(texture[0][ind][j]);
						else if (k==2) out.print( normal[0][ind][j]);
					}
					else
						out.print ("0");

					if (i<size-1 || j<(k==1 ? 1: 2))
						out.print (", ");
				}
				out.println ();
			}
			out.println ("};\n");
		}

		size = orderedIndices.length;
		out.println ("static unsigned int "+methodName+"Indices["+size+"] = {");

		int c = 0;
		for (int g=0; g<groups.size(); g++)
		{
			for (int i=0; i<groups.get(g)[1]; i++) {
				out.print(indent);

				for (int j=0; j<groups.get(g)[0]; j++, c++)
				{
					out.print (orderedIndices[c]);
					if (c < size-1 || j<groups.get(g)[0]-1)
						out.print(", ");
				}
				out.println ();
			}
		}
		out.println ("};\n");
	}

	static void animateObjFile () throws IOException {

		if (!wave && fileCounter==1)
			return; //nothing to animate

		if (!initAnimateRoutine) {
			out.println("\nvoid\nAnimate"+methodName+"(float t)\n{");

			if (fileCounter>1)
				out.println(indent+"t = fmodf(t, "+fileCounter+");");

			out.println(indent+"int   ti = (int) t;");
			if (fileCounter>1) {
				out.println(indent+"float dt = t-ti;");
				if (!linear) {
					out.println(indent+"float dt2 = dt*dt;");
					out.println(indent+"float dt3 = dt2*dt;");
				}
			}
			if (wave) out.println(indent+"float w = 2*PI*(t-ti);");

			if (fileCounter>1)
				out.println("\n"+indent+"switch (ti) {");

			//out.println("int time = (int) fmodf(t,"+fileCounter+");");
			initAnimateRoutine = true;
		}


		for (int j=0; j<fileCounter; j++) {

			for (int c=0; c<indices.size(); c++) {


				int ptInd = indices.get(c)[0];
				int ptIndDup = -1;

				for (int c2=0; c2<c; c2++)
				{
					if (ptInd==indices.get(c2)[0])
					{
						ptIndDup = c2;
						break;
					}
				}

				int normalInd = indices.get(c)[2];
				int normalIndDup = -1;

				for (int c2=0; c2<c; c2++)
				{
					if (normalInd==indices.get(c2)[2])
					{
						normalIndDup = c2;
						break;
					}
				}



				if (c==0 && fileCounter>1) {
					if (j>0) out.println ("\tbreak;\n");
					out.println(indent+"case("+j+"):");
				}


				for (int i=0; i<3; i++) {

					String pre = (fileCounter>1 ? indent2: indent);
					String preN = pre;

					if (fileCounter>1)
					{ //save space so that compiling does not take so long
						pre  += methodName+"Points[" +(i+3*c)+"] =";
						preN += methodName+"Normals["+(i+3*c)+"] =";
					}
					else
					{
						pre  += methodName+"Points["+i+"+3*"+c+"] =";
						preN += methodName+"Normals["+i+"+3*"+c+"] =";
					}

					if (ptIndDup<0)     pre  += (pts[j][ptInd][i]);
					if (normalIndDup<0) preN += (normal[j][normalInd][i]);


					int j2 = (j+1)%fileCounter;

					if (changeInPts[ptInd][i])
					{
						if (ptIndDup<0) {
							if (linear) { //linear interpolation
								float A = (pts[j2][ptInd][i]-pts[j][ptInd][i]);
								if (A==0)
									out.println (pre+";");
								else
									out.println (pre+"+dt*"+A+";");
							}
							else { //cubic spline interpolation

								float B   = coefficientB (j, ptInd, i, pts);
								float Bp1 = coefficientB (j2, ptInd, i, pts);

								float A = coefficientA(B, Bp1, j, ptInd, i, pts);
								float C = coefficientC(B, Bp1);

								if (A!=0 && B!=0 && C!=0)
									out.println (pre+"+dt*"+A+"+dt2*"+B+"+dt3*"+C+";");
								else
									out.println (pre+";");
							}
						}
						else
						{
							if (fileCounter>1)
								out.println (pre+methodName+"Points["+(i+3*ptIndDup)+"];");
							else
								out.println (pre+methodName+"Points["+i+"+3*"+ptIndDup+"];");
						}
					}

					if (changeInNormals[normalInd][i])
					{
						if (normalIndDup<0) {
							if (linear) { //linear interpolation
								float A = (normal[j2][normalInd][i]-normal[j][normalInd][i]);
								if (A==0)
									out.println (preN+";");
								else
									out.println (preN+"+dt*"+A+";");
							}
							else { //cubic spline interpolation

								float B   = coefficientB (j, normalInd, i, normal);
								float Bp1 = coefficientB (j2, normalInd, i, normal);

								float A = coefficientA(B, Bp1, j, normalInd, i, normal);
								float C = coefficientC(B, Bp1);

								if (A!=0 && B!=0 && C!=0)
									out.println (preN+"+dt*"+A+"+dt*dt*"+B+"+dt*dt*dt*"+C+";");
								else
									out.println (preN+";");
							}
						}
						else
						{
							if (fileCounter>1)
								out.println (preN+methodName+"Normals["+(i+3*normalIndDup)+"];");
							else
								out.println (preN+methodName+"Normals["+i+"+3*"+normalIndDup+"];");
						}
					}


					if (i == 2) {

						if (wave && pts[j][ptInd][0]>=xOffset*scaleFactor) {

							if (ptIndDup<0)
							{
							float A = frequencyFactor*(pts[j][ptInd][0]/scaleFactor-xOffset);
							A = (float) (((double) A)%(2*Math.PI));
							if (A>0) A-=2*Math.PI;


							out.println(pre+"+"+(amplitude*(pts[j][ptInd][0]-xOffset*scaleFactor))+
									"*sinf(w"+(A>=0 ? "+" : "")+A+");");
							}
							else {
								out.print ((fileCounter>1 ? indent2: indent));
								out.println (methodName+"Points["+i+"+3*"+c+"] = "+methodName+"Points["+i+"+3*"+ptIndDup+"];");
							}
						}
					}
				}


			}
		}
		if (initAnimateRoutine) {
			if (fileCounter>1)
				out.println ("\tbreak;\n    }");
			out.println ("}\n");
		}

	}

	static void vertexArrayObjFile () {


		if (wire) out.println("\nvoid\nDraw"+methodName+"(int wire)\n{");
		else  out.println("\nvoid\nDrawAnimated"+methodName+"()\n{");

		if (groups.size()== 0) return;

		int c = 0;
		Boolean prevUseNormals = false;
		Boolean useNormals = false;

		int materialIndex = -1;

		out.println (indent+"glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);");

		for (int i=0; i<groups.size(); i++)
		{
			if (i!=0) out.println ();

			int numElements = groups.get(i)[0]*groups.get(i)[1];

			String type = "GL_POLYGON";
			if (groups.get(i)[0]==1)      type="GL_POINTS";
			else if (groups.get(i)[0]==2) type="GL_LINE_LOOP";
			else if (groups.get(i)[0]==3) type="GL_TRIANGLES";
			else if (groups.get(i)[0]==4) type="GL_QUADS";

			prevUseNormals = useNormals;
			useNormals = (indices.get(orderedIndices[c])[1]<0);

			if (prevUseNormals!=useNormals)
			{
				if (useNormals) out.println(indent+"glEnableClientState(GL_NORMAL_ARRAY);");
				else out.println(indent+"glDisableClientState (GL_NORMAL_ARRAY);");
			}

			if (i==0)
			{
				out.println(indent+"glEnableClientState(GL_VERTEX_ARRAY);");
				out.println(indent+"glVertexPointer (3, GL_FLOAT, 0, "+methodName+"Points);");
				out.println(indent+"glNormalPointer (GL_FLOAT, 0, "+methodName+"Normals);\n");
			}

			int go = (i==0 ? -1 : groups.get(i-1)[2]);
			if (labelSeparateObjects && separateObjects &&
				groups.get(i)[2]!=go && groups.get(i)[2]>=0)
				out.println (indent+"//"+objectNames.get(groups.get(i)[2]));

			if (groups.get(i)[3]>=0)
			{
				if (materialIndex!=groups.get(i)[3])
				{
					materialIndex = groups.get(i)[3];
					printUseMaterial(materialIndex);
				}
			}
			else if (twoSided)
			{
				 out.println (indent+"glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);");
				 if (setCullFace) out.println (indent+"glEnable(GL_CULL_FACE);");
				 twoSided = false;
			}

			if (wire) out.println (indent+(i==0 ? "GLenum ": "")+"cap = wire ? GL_LINE_LOOP : "+type+";");

			out.println(indent+"glDrawElements("+(wire ? "cap": type)+", "+(numElements)+", GL_UNSIGNED_INT, &("+methodName+"Indices["+(c)+"]));");

			c+= numElements;
		}

		if (useNormals) out.println(indent+"glDisableClientState (GL_NORMAL_ARRAY);");

		out.println (indent+"glPopAttrib();");
		out.println("}\n");
	}

	static void printUseMaterial(int materialIndex)
	{
		if (!disableColorMaterial) out.println(indent+"glDisable (GL_COLOR_MATERIAL);");
		disableColorMaterial = true;

		out.println(indent+"glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);");

		String renderSide = glRenderSide;
		if (materialNames.get(materialIndex).substring(0,Math.min("userDefined_DS".length(), materialNames.get(materialIndex).length())).equalsIgnoreCase("userDefined_DS"))
		{
			renderSide = "GL_FRONT_AND_BACK";

			if (!twoSided) {
				out.println (indent+"glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 1.0f);");
				if (setCullFace) out.println (indent+"glDisable(GL_CULL_FACE);");
			}
			twoSided = true;
		}
		else if (twoSided)
		{
			 out.println (indent+"glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, 0.0f);");
			 if (setCullFace) out.println (indent+"glEnable(GL_CULL_FACE);");
			 twoSided = false;
		}

		for (int j=0; j<4; j++) {
			if (materialAttributes.get(materialIndex)[j]) {
				out.println (indent+"glMaterialfv ("+renderSide+", GL_"+attributes[j].toUpperCase()+"" +
						", "+materialNames.get(materialIndex)+"_"+attributes[j]+");");
			}
		}
	}


	static void initEndObjFile() {
		if (!initDrawingRoutine) {
			printOpenGLInitEnd();
		}

	}
	static void printOpenGLInitEnd() {
		//init
		out.println("void\ninitDraw"+methodName+"(float *color)\n{");
		out.println(openGLinit+"\n");

		for (int i=0; i<materialNames.size(); i++)
		{
			float factor = 1.0f;
			Boolean copyColor = false;
			if (materialNames.get(i).substring(0,Math.min("userDefined_DS".length(), materialNames.get(i).length())).equalsIgnoreCase("userDefined_DS"))
			{
				try {
					if (materialNames.get(i).charAt("userDefined_DS".length())=='_')
						factor = 0.01f*Float.parseFloat(materialNames.get(i).substring("userDefined_DS".length()+1, materialNames.get(i).length()));
				}
				catch (Exception ex) { factor = 1.0f; }
				copyColor = true;
			}
			else if (materialNames.get(i).substring(0,Math.min("userDefined".length(), materialNames.get(i).length())).equalsIgnoreCase("userDefined"))
			{
				try {
					if (materialNames.get(i).charAt("userDefined".length())=='_')
						factor = 0.01f*Float.parseFloat(materialNames.get(i).substring("userDefined".length()+1, materialNames.get(i).length()));
				}
				catch (Exception ex) { factor = 1.0f; }
				copyColor = true;
			}

			if (copyColor)
			{
				out.println (indent+"copyColor((float *)"+materialNames.get(i)+"_"+attributes[1]+", color, "+(ambientFactor*factor)+");");
				out.println (indent+"copyColor((float *)"+materialNames.get(i)+"_"+attributes[2]+", color, "+(diffuseFactor*factor)+");");
				out.println (indent+"copyColor((float *)"+materialNames.get(i)+"_"+attributes[3]+", color, "+(specularFactor*factor)+");");
			}
		}

		out.println("}\n");


		//fin
		out.println("void\nfinDraw"+methodName+"()\n{");
		out.println(openGLfin+"\n}\n");

		out.println();
	}


	//spline S(t) = y0 + A*t + B*t^2 + C*t^3;
	static float coefficientB(int j, int v, int i, float[][][] data) { //B(t+1)
		j%=fileCounter;
		if (j<0) j+=fileCounter;

		if (j==0 || j==fileCounter-1) return 0; //natural cubic spline

		int jm1 = j-1; if (jm1<0) jm1+=fileCounter;
		int jm2 = j-2; if (jm2<0) jm2+=fileCounter;
		int jm3 = j-3; if (jm3<0) jm3+=fileCounter;

		if (j==1) {

			float c = 3*(data[fileCounter-1][v][i]-2*data[fileCounter-2][v][i]+data[fileCounter-3][v][i]);

			float d=1.0f/4;

			for (int k=2; k<fileCounter-1; k++) {

				c=3*(data[fileCounter-k][v][i]-2*data[fileCounter-k-1][v][i]+data[fileCounter-k-2][v][i])-d*c;

				d = 1.0f/(4.0f-d);
			}

			c*=d;
			return c;
		}

		return 3*(data[j][v][i]-2*data[jm1][v][i]+data[jm2][v][i])
			   -coefficientB(jm2, v, i, data)-4*coefficientB(jm1, v, i, data);
	}
	static float coefficientA(float B, float Bp1, int j, int v, int i, float[][][] data) { //A(t)
		return (data[(j+1)%fileCounter][v][i]-data[j][v][i])-(Bp1+2*B)/3;
	}
	static float coefficientC(float B, float Bp1) { //C(t)
		return (Bp1-B)/3;
	}
}

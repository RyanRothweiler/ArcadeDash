using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class LevelCreator : MonoBehaviour
{

	public List<GameObject> enemyFabs;
	public GameObject wallFab;


	private bool levelGenerated = false;
	//TODO maybe remove this
	private Vector2 levelBounds = new Vector2(15, 8);

	public void Start()
	{
		if (!levelGenerated)
		{
			levelGenerated = true;
			GenerateLevel(GlobalState.instance.enemyCount);
		}
	}

	public void GenerateLevel(int enemyCount)
	{

		// wall generation
		int wallCount = Random.Range(8, 13);
		int wallsPlaced = 0;
		while (wallsPlaced < wallCount)
		{
			// get all walls before we add another
			Wall[] walls = FindObjectsOfType(typeof(Wall)) as Wall[];

			GameObject newWall = Instantiate(wallFab) as GameObject;

			Vector3 newPos = new Vector3(Random.Range(-levelBounds.x, levelBounds.x),
			                             Random.Range(-levelBounds.y, levelBounds.y),
			                             0);
			newWall.transform.position = newPos;


			Vector2 scaleDir = Vector2.zero;
			if (Random.Range(0, 2) == 1)
			{
				scaleDir = new Vector2(1, 0);
			}
			else
			{
				scaleDir = new Vector2(0, 1);
			}
			float randomScale = Random.Range(0, 5);
			Vector3 newScale = new Vector3(0.5f + (randomScale * scaleDir.x), 0.5f + (randomScale * scaleDir.y), 1);
			newWall.transform.localScale = newScale;

			bool wallValid = true;
			foreach (Wall wall in walls)
			{
				Bounds newWallBounds = newWall.GetComponent<Collider2D>().bounds;
				if (wall.GetComponent<Collider2D>().bounds.Intersects(newWallBounds))
				{
					wallValid = false;
					Destroy(newWall);
				}
			}
			if (wallValid)
			{
				wallsPlaced++;
			}
		}


		Wall[] totalWalls = FindObjectsOfType(typeof(Wall)) as Wall[];

		// enemy generation
		int enemiesMade = 0;
		while (enemiesMade < enemyCount)
		{
			GameObject enemyFab = enemyFabs[Random.Range(0, enemyFabs.Count)];
			GameObject newEnemy = Instantiate(enemyFab);

			Vector3 newPos = new Vector3(Random.Range(-levelBounds.x, levelBounds.x),
			                             Random.Range(-levelBounds.y, levelBounds.y),
			                             0);
			newEnemy.transform.position = newPos;

			bool enemyValid = true;
			foreach (Wall wall in totalWalls)
			{
				Bounds enemyBounds = newEnemy.GetComponent<Collider2D>().bounds;
				if (wall.GetComponent<Collider2D>().bounds.Intersects(enemyBounds))
				{
					enemyValid = false;
					Destroy(newEnemy);
					break;
				}
				if (Vector3.Distance(newEnemy.transform.position, PlayerController.instance.transform.position) < 4.5f)
				{
					enemyValid = false;
					Destroy(newEnemy);
				}
			}

			if (enemyValid)
			{
				enemiesMade++;

				// this is custom enemy behavior
				Enemy enemy = newEnemy.GetComponent<Enemy>();
				switch (enemy.type)
				{
					case EnemyType.WallCrawler:
					{

						GameObject baseWall = null;
						bool foundWall = false;
						while (!foundWall)
						{
							baseWall = totalWalls[Random.Range(0, totalWalls.Length)].gameObject;
							if (baseWall.transform.localScale.y < 8)
							{
								foundWall = true;
							}
						}


						bool verticalWall;
						if (baseWall.transform.localScale.x > baseWall.transform.localScale.y)
						{
							verticalWall = true;
						}
						else
						{
							verticalWall = false;
						}

						if (verticalWall)
						{
							newEnemy.transform.position = baseWall.transform.position - new Vector3(1, 0, 0);
							WallCrawler crawler = enemy.GetComponent<WallCrawler>();

							crawler.topTarget.transform.position = crawler.transform.position +
							                                       new Vector3(0, baseWall.transform.localScale.x, 0);
							crawler.bottomTarget.transform.position = crawler.transform.position -
							        new Vector3(0, baseWall.transform.localScale.x, 0);
						}
						else
						{
							newEnemy.transform.position = baseWall.transform.position - new Vector3(0, 1, 0);
							WallCrawler crawler = enemy.GetComponent<WallCrawler>();
							crawler.transform.Rotate(0, 0, -90);

							crawler.topTarget.transform.position = crawler.transform.position +
							                                       new Vector3(baseWall.transform.localScale.y, 0, 0);
							crawler.bottomTarget.transform.position = crawler.transform.position -
							        new Vector3(baseWall.transform.localScale.y, 0, 0);
						}


						break;
					}
				}
			}
		}
	}
}
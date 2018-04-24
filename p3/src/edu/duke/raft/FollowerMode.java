package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

// notes:
// Respond to RPCs from candidates and leaders
// If election timeout elapses without receiving AppendEntries
// RPC from current leader or granting vote to candidate:
// convert to candidate

/* 5.2
 * A server remains in follower state as long as it receives valid
 * RPCs from a leader or candidate. Leaders send periodic
 * heart beats (AppendEntries RPCs that carry no log entries)
 * to all followers in order to maintain their authority. If a
 * follower receives no communication over a period of time
 * called the election timeout, then it assumes there is no viable
 * leader and begins an election to choose a new leader.
 */

public class FollowerMode extends RaftMode {
	Timer electionTimer;
	final int electionTimerID = 1;

	public void go () {
		synchronized (mLock) {
			log("switched to follower mode.");
			
			int electionTimeout;
			if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
				electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
			}
			
			electionTimer = scheduleTimer(electionTimeout, electionTimerID);
		}
	}
	
	/* Reply false if term < currentTerm (§5.1)
	 * If votedFor is null or candidateId, and candidate’s log is at least as up-to-date as receiver’s log, grant vote (§5.2, §5.4)
	 * 
	 * Raft determines which of two logs is more up-to-date
	 * by comparing the index and term of the last entries in the
	 * logs. If the logs have last entries with different terms, then
	 * the log with the later term is more up-to-date. If the logs
	 * end with the same term, then whichever log is longer is
	 * more up-to-date. 
	 */

	// @param candidate’s term
	// @param candidate requesting vote
	// @param index of candidate’s last log entry
	// @param term of candidate’s last log entry
	// @return 0, if server votes for candidate; otherwise, server's
	// current term
	public int requestVote (int candidateTerm,
			int candidateID,
			int lastLogIndex,
			int lastLogTerm) {
		synchronized (mLock) {
			log("vote request from server: " + candidateID + "." + candidateTerm);
			
			boolean candidateNewerTerm = candidateTerm >= mConfig.getCurrentTerm();
			boolean haventVoted = (mConfig.getVotedFor() == 0 /*  || mConfig.getVotedFor() == candidateID */);
			boolean candidateLogUpToDate = (lastLogTerm == mLog.getLastTerm()) ? lastLogIndex >= mLog.getLastIndex() : lastLogTerm > mLog.getLastTerm();
			
			if (candidateNewerTerm && haventVoted && candidateLogUpToDate) {
				mConfig.setCurrentTerm(candidateTerm, candidateID);
				log("voted for server: " + candidateID + "." + candidateTerm);
				return 0; // Vote for server
			} else {
				log("didn't vote for server: " + candidateID + "." + candidateTerm);
				mConfig.setCurrentTerm(mConfig.getCurrentTerm(), 0); //Do we vote for anyone? Do we vote for ourselves?
				return mConfig.getCurrentTerm(); // No vote, return term
			}
		}
	}
	
	/*
	 * Receiver implementation:
	 * 1. Reply false if term < currentTerm (§5.1)
 	 * 2. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm (§5.3)
     * 3. If an existing entry conflicts with a new one (same index but different terms), delete the existing entry and all that follow it (§5.3)
	 * 4. Append any new entries not already in the log
	 * 5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry) !!!!
	 */
	
	// @param leader’s term
	// @param current leader
	// @param index of log entry before entries to append
	// @param term of log entry before entries to append
	// @param entries to append (in order of 0 to append.length-1)
	// @param index of highest committed entry
	// @return 0, if server appended entries; otherwise, server's
	// current term
	public int appendEntries (int leaderTerm,
			int leaderID, // so follower can redirect clients?
			int prevLogIndex,
			int prevLogTerm,
			Entry[] entries,
			int leaderCommit) {
		synchronized (mLock) {
			log("append entries request from P"+leaderID+"."+leaderTerm);
			int lowestCommonIndex = Math.max(0, Math.min(prevLogIndex, mLog.getLastIndex()));
			int lowestCommonTerm = mLog.getEntry(lowestCommonIndex).term;
			
			// investigate
			if (leaderTerm >= mConfig.getCurrentTerm()) {
				log("recognized heartbeat from leader P"+leaderID);
				electionTimer.cancel();
				
				int electionTimeout;
				if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
					electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
				}
				
				electionTimer = scheduleTimer(electionTimeout, electionTimerID);
				
				// Repair Log
				if (leaderTerm >= mConfig.getCurrentTerm()) {
					mConfig.setCurrentTerm(leaderTerm, 0); //Update to make sure term is correct. 
					log("updated term to leader term");
				}
				
				if (lowestCommonTerm == prevLogTerm && entries.length != 0) {
					if (mLog.insert(entries, prevLogIndex, prevLogTerm) == -1) {
						log("insert operation failed!"); // break?
					} else {
						log("inserted entries");
					}
				}
				
				mCommitIndex = Math.min(leaderCommit, mLog.getLastIndex());

				return 0;
			} else {
				log("rejected AppendEntries.");
				return mConfig.getCurrentTerm();
			}
		}
	}  

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				log("time is up! transition to candidate");
				electionTimer.cancel(); // If timer goes off, become a candidate
				RaftServerImpl.setMode(new CandidateMode());
			}
		}
	}

	public void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + " (follower): " + message);
	}
}


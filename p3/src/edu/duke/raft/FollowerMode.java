package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class FollowerMode extends RaftMode {
	Timer electionTimer;
	private final int electionTimerID = 1;

	public void go () {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm();
			System.out.println ("S" + 
					mID + 
					"." + 
					term + 
					": switched to follower mode.");

			int electionTimeout; 
			if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
				electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
			}

			electionTimer = scheduleTimer(electionTimeout, electionTimerID);
		}
	}

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
			electionTimer.cancel();
			
			log("Vote Request from Server: " + candidateID + "." + candidateTerm);

			int currentTerm = mConfig.getCurrentTerm ();
			
			boolean candidateOutdatedTerm = candidateTerm < currentTerm;
			boolean haventVoted = (mConfig.getVotedFor() == 0 || mConfig.getVotedFor() == candidateID);
			boolean candidateLogUpToDate = (lastLogTerm == mLog.getLastTerm()) ? lastLogIndex > mLog.getLastIndex() : lastLogTerm > mLog.getLastTerm();
			
			int electionTimeout; 
			if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
				electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
			}

			electionTimer = scheduleTimer(electionTimeout, electionTimerID);
			
			if(candidateOutdatedTerm) {
				log("Didn't vote for Server: " + candidateID + "." + candidateTerm);
				mConfig.setCurrentTerm(currentTerm, 0);
				return currentTerm;
			} else if (haventVoted && candidateLogUpToDate) {
				log("Voted for Server: " + candidateID + "." + candidateTerm);
				mConfig.setCurrentTerm(candidateTerm, candidateID);
				return 0;
			}
			log("Didn't vote for Server: " + candidateID + "." + candidateTerm);
			mConfig.setCurrentTerm(currentTerm, 0);
			return currentTerm;
		}
	}


	// @param leader’s term
	// @param current leader
	// @param index of log entry before entries to append
	// @param term of log entry before entries to append
	// @param entries to append (in order of 0 to append.length-1)
	// @param index of highest committed entry
	// @return 0, if server appended entries; otherwise, server's
	// current term
	public int appendEntries (int leaderTerm,
			int leaderID,
			int prevLogIndex,
			int prevLogTerm,
			Entry[] entries,
			int leaderCommit) {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm ();
			int result = term;
			return result;
		}
	}  

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				electionTimer.cancel();
				RaftServerImpl.setMode(new CandidateMode());
			}

		}
	}
	
	private void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + " (Follower Mode): " + message);
	}

}

